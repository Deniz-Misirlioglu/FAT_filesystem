#include <string>
#include <sstream>
#include <iostream>
#include <array>
#include <memory>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <cstddef>
#include <bitset>
#include <cmath>

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

enum class [[nodiscard]] Status
{
    Ok,
    Error
};

// Size of the drive/array
constexpr int kMaxDriveSize(1200);
// Max filename length in the filesystem
constexpr int kMaxFilenameSize(16);
// Max number of files in the directory
constexpr int kMaxNumberFiles(5);
// The number of bytes each block is on the drive
constexpr int kBlockSize(30);
// The number of elements in the File Allocation Table
constexpr int kTableSize(30);
// The end of file marker in the file allocation table
constexpr int kEndOfFile(-1);

constexpr int bufferOffset(10);

struct FileEntry
{
    std::array<char, kMaxFilenameSize> name{'\0'};
    int starting_block{0};
    int size{0};
    double last_modified{0.0};
};

/*he main structure that holds the FATfilesystem
In this scruct, all methods are available such as
Create file - create
remove - rm
listDirectory - ls
See the contents - cat
DiskUsage - du
renameFile - mv
New Method - fat - which prints the fat table
*/
struct FATFilesystem
{
    // This is the drive we will be reading/writing to/from, and will store our filesystem
    std::array<std::byte, kMaxDriveSize> drive;

    // Memory versions of the root directory/FAT table, use memcpy to save/load to/from the drive
    std::array<FileEntry, kMaxNumberFiles> root_directory_in_memory;
    std::array<int, kTableSize> fat_table_in_memory;

    int fileIndex;
    int numOfFiles;
    int sizeDiff;
    int blockIndex;
    int nameInitizliation;
    int fatInitialization;
    int fatTableIndex;
    FATFilesystem()
    {
        nameInitizliation = 0;
        fatInitialization = 0;
        blockIndex = 10;
        fatTableIndex = 0;
        fileIndex = 300;
        numOfFiles = 0;
        sizeDiff = 0;
        std::memcpy(&drive[0], &root_directory_in_memory, kMaxDriveSize);
        std::memcpy(&drive[180], &fat_table_in_memory, kMaxDriveSize);
    }

    /* The method which lists the current available directories in the fileSystem
    and is identical to the ls command in linux
    */
    Status listDirectory()
    {
        {
            if (numOfFiles == 0)
            {
                return Status::Ok;
            }
            std::cout << "total " << sizeDiff + 300 << " bytes\n";
            std::cout << "Name\t\t\tStart Block\t\tSize\t\tLast Modified\n";
            for (int i = 0; i < kMaxNumberFiles - 1; i++) // Indexing the root_directory
            {
                if (root_directory_in_memory[i].name[0] != '\0')
                {
                    std::string fullName = "";
                    // uint length = root_directory_in_memory[i].name.size();
                    uint l = 0, count = 0;
                    while (root_directory_in_memory[i].name[l])
                    {
                        count++;
                        l++;
                    }
                    for (uint j = 0; j < count; ++j) // Looping to get the name of the current directory
                    {
                        if (root_directory_in_memory[i].name[j] != '\0')
                            fullName += root_directory_in_memory[i].name[j];
                    }
                    if (fullName != "") // If the name is not empty, print it out in the ls directory
                    {
                        std::cout << fullName;
                        std::cout << "\t\t";
                        std::cout << (root_directory_in_memory[i].starting_block)
                                  << "\t\t\t" << root_directory_in_memory[i].size
                                  << " bytes\t";
                        std::time_t t(root_directory_in_memory[i].last_modified);
                        std::cout << ctime(&t);
                    }
                }
            }
            return Status::Ok;
        }
        cout << "Error: something went wrong" << endl;
        return Status::Error;
    }

    /*he method which creates a file, stores the value inside of the drive,
    and stores the blocks inside of the FAT table.
    This method returns Status::OK when the file was succesfully created, and Error when not
    */
    Status createFile(const std::string &filename, unsigned int filesize,
                      std::byte value)
    {
        int b_size = std::ceil(filesize / 30.0);

        if (nameInitizliation == 0) // Initializing all of the names to null when creating the first file
        {
            for (int i = 0; i < 4; i++)
            {
                root_directory_in_memory[i].name[0] = '\0';
            }
            nameInitizliation++;
        }
        if (fatInitialization == 0) // Initializing the fat table when creating the first file
                                    // All of the indexes will be set to (-2)
        {
            for (int i = 0; i < kTableSize; i++)
            {
                fat_table_in_memory[i] = -2;
            }
            fatInitialization++;
        }
        // ERROR CHECKING
        for (uint i = 0; i < filename.size(); i++)
        {
            if (isalpha(filename[i]) == 0)
            {
                if (filename[i] != '.')
                {
                    cout << "Error: string has non-alphabetic characters" << endl;
                    return Status::Error;
                }
            }
        }
        // uint endfile = 0;
        //  Checking the size of the drive
        if (filesize + sizeDiff > 1024)
        {
            cout << "Error: Array is full" << endl;
            return Status::Error;
        }

        if (numOfFiles >= kMaxNumberFiles) // checking if the max number of files has been reached
        {
            cout << "Max files Reached" << endl;
            return Status::Error;
        }
        if (filename.length() > kMaxFilenameSize) // Checking if the name is longer than the max lane
        {
            return Status::Error;
        }
        for (auto &s : root_directory_in_memory)
        {

            if (s.name[0] == '\0')
            {
                char arr[filename.length() + 1];
                strcpy(arr, filename.c_str());

                for (unsigned int i = 0; i < filename.length(); i++)
                {
                    s.name[i] = arr[i];
                }

                s.name[filename.length()] = '\0';

                time_t now = time(0);
                s.last_modified = now;

                s.starting_block = getStartingBlock(); // Calculating the Starting Block using the getStartingBlock method
                s.size = filesize;

                int start = s.starting_block;

                int filled = 0; // The filled index which keeps tracks of the filled blocks in the table
                int index = s.starting_block - bufferOffset;
                while (filled < b_size) // If the filled blocks = the number of blocks of the file
                {
                    filled++;

                    if (filled == b_size)
                    { // Setting the data in the fat table with the correct indexs
                        fat_table_in_memory[index] = -1;
                    }
                    else
                    {
                        int next_block = findBlock(start); // Finding the next open block in the table
                        fat_table_in_memory[index] = next_block;
                        index = next_block - bufferOffset;
                        start = next_block;
                    }
                }

                // fatTableIndex += b_size;
                // vector<int> fat_chain = get_fat_chain(s.starting_block, fat_table_in_memory);

                // for(int i = 0; i < fat_chain.size(); i++) {
                //     cout << fat_chain.at(i) << " ";
                // }
                Status stat = writingDrive(s.starting_block, s.size, value);
                if (stat == Status::Ok)
                    break;
            }
        }
        // fileIndex = endfile;
        numOfFiles++;
        sizeDiff = filesize + sizeDiff;

        return Status::Ok;
    }
    /*
    This method is used to determine the next open block that is available in the
    fat table. This method returns an int of the next block.
    */
    int findBlock(int block)
    {
        for (int i = block - bufferOffset; i < kTableSize; i++)
        {
            if (fat_table_in_memory[i] == -2)
            {
                return i + bufferOffset + 1;
            }
        }
        return 0;
    }

    /* This method is used to write to the drive with parameters
    of the given start block, the size of the file, and the byte value
    This method returns void
    */
    Status writingDrive(int start, int size, std::byte value)
    {
        int b_size = size % kBlockSize;
        if (b_size == 0)
            b_size = 30;
        std::vector<int> fat_chain = get_fat_chain(start, fat_table_in_memory); // Getting the fat chain into a vector

        for (unsigned int i = 0; i < fat_chain.size(); i++)
        {
            auto &c = fat_chain[i];
            int driveS = kBlockSize * c;
            if (i == fat_chain.size() - 1)
            {
                for (int i = driveS; i < driveS + b_size; i++)
                {
                    drive[i] = value; // Writing to the block with the given value
                }
            }
            else
            {
                for (int i = driveS; i < driveS + kBlockSize; i++)
                {
                    drive[i] = value; // Writing to the block with the given value
                }
            }
        }
        return Status::Ok;
    }

    /*This method removes a file from the root_directory_in_memory
    This method returns OK if the file was succesfully removed,
    or Error if it was not*/
    Status remove(const std::string &filename)
    {
        int filesRemoved = 0;
        for (int i = 0; i < kMaxNumberFiles - 1; i++)
        {
            string fullName = "";
            uint l = 0, count = 0;
            while (root_directory_in_memory[i].name[l])
            {
                count++;
                l++;
            }
            for (uint j = 0; j < count; j++)
            {
                // if (root_directory_in_memory[i].name[j] != '\0')
                fullName += root_directory_in_memory[i].name[j]; // Adding to the name
            }

            if (fullName == filename) // Checking if a filename is equal to a name in the directory
            {
                Status s;
                // Nulling every element of the File, and removing the indexs in the fat table
                s = remove_in_fat(root_directory_in_memory[i].starting_block);
                if (s == Status::Ok)
                {
                    sizeDiff -= root_directory_in_memory[i].size; // NULL SETTING OF THE FILE
                    root_directory_in_memory[i].size = '\0';
                    root_directory_in_memory[i].starting_block = '\0';
                    root_directory_in_memory[i].last_modified = '\0';
                    filesRemoved++;
                    for (uint j = 0; j < count; j++)
                    {
                        root_directory_in_memory[i].name[j] = '\0';
                    }
                }
            }

            if (filesRemoved != 0)
            {
                numOfFiles--;
                return Status::Ok;
            }
        }
        cout << "Error: file does not exist" << endl;
        return Status::Error;
    }

    /*This method takes the start index and removes every element in the fat table chain
    This method returns void
    */
    Status remove_in_fat(int start)
    {
        vector<int> a = get_fat_chain(start, fat_table_in_memory); // Using the get fat chain method
        for (uint i = 0; i < a.size(); i++)
        {
            int k = a.at(i);
            fat_table_in_memory[k - bufferOffset] = -2; // Setting the index to the original starting value of -2
        }
        return Status::Ok;
    }

    /* This method renames a file to a another name. You are unable to
    rename a file unless all of the characters are alphabetic.
    This method returns Ok on success  rename or Error on nonsuccesul rename
    */
    Status rename(const std::string &filename, const std::string &new_filename)
    {

        for (uint i = 0; i < filename.size(); i++)
        {
            if (isalpha(filename[i]) == 0)
            {
                // Error checking but allowing a period
                if (filename[i] != '.')
                {
                    cout << "Error: string has non-alphabetic characters" << endl;
                    return Status::Error;
                }
            }
        }
        int filesChanged = 0; // Checking the files that have been chaned, used to determine the Status
        for (int i = 0; i < kMaxNumberFiles - 1; i++)
        {
            string fullName = "";
            uint l = 0, count = 0;
            while (root_directory_in_memory[i].name[l])
            {
                count++;
                l++;
            }
            for (uint j = 0; j < count; j++)
            {
                if (root_directory_in_memory[i].name[j] != '\0')
                    fullName += root_directory_in_memory[i].name[j];
            }
            if (fullName == filename) // Checking if the filename is equal to a filename in the directory
            {

                for (unsigned int a = 0; a < filename.length(); a++)
                {
                    root_directory_in_memory[i].name[a] = '\0'; // Setting the old name to null
                }

                root_directory_in_memory[i].name[filename.length()] = '\0';

                for (uint k = 0; k < new_filename.size(); k++)
                {
                    root_directory_in_memory[i].name[k] = new_filename[k]; // Setting the new name to the new_filename
                    filesChanged++;
                }
                root_directory_in_memory[i].name[new_filename.length()] = '\0';
            }
        }
        // Printing the status as either Ok or Error depending if a file was correctly renamed
        if (filesChanged != 0)
        {
            return Status::Ok;
        }
        cout << "Error: file does not exist" << endl;
        return Status::Error;
    }

    /* This method is the print command which prints out the contents in the file.
    This method reutnrs Ok on successful print, or error on unsuccessful print.
    */
    Status print(const std::string &filename)
    {
        // TODO
        int filesPrinted = 0;

        for (int i = 0; i < kMaxNumberFiles - 1; i++)
        {
            string fullName = "";
            uint l = 0, count = 0;
            while (root_directory_in_memory[i].name[l])
            {
                count++;
                l++;
            }
            for (uint j = 0; j < count; j++)
            {
                if (root_directory_in_memory[i].name[j] != '\0')
                    fullName += root_directory_in_memory[i].name[j];
            }
            if (filename == fullName) // Checking if the filename equals to the fullname
            {
                int b_size = root_directory_in_memory[i].size % kBlockSize;
                if (b_size == 0)
                    b_size = 30;
                std::vector<int> fat_chain = get_fat_chain(root_directory_in_memory[i].starting_block, fat_table_in_memory);
                for (unsigned int i = 0; i < fat_chain.size(); i++)
                {
                    auto &c = fat_chain[i];
                    int driveB = kBlockSize * c;
                    filesPrinted++;
                    if (i == fat_chain.size() - 1)
                    {
                        for (int k = driveB; k < driveB + b_size; k++)
                        {
                            cout << static_cast<char>(drive[k]); // Setting the drive to the value of the file
                        }
                    }

                    else
                    {
                        for (int k = driveB; k < driveB + kBlockSize; k++)
                        {
                            cout << static_cast<char>(drive[k]); // Setting the drive to the value of the file
                        }
                    }
                }
            }
        }
        if (filesPrinted != 0)
        {
            cout << endl;
            return Status::Ok;
        }
        cout << "Error: file does not exist" << endl;
        return Status::Error;
    }
    /* This method is used to determine the amount of disk space left on the drive
    and will return Status::Ok if successful, or Error if unsuccessful*/
    Status diskUsage()
    {
        try
        { // Printing out the calculated disk usage which is equal to the max of 1024 bytes
            double d = (double)sizeDiff / 1024 * 100;
            cout << sizeDiff << "/1024 ";
            fprintf(stdout, "%.2f", d);
            cout << "% full" << endl;
            return Status::Ok;
        }
        catch (int d)
        {
            return Status::Error;
        }
    }
    /* This method is used to determine the starting block of the file, by looking
    which index is equal to -2 as this is the default value in the fat table
    */
    int getStartingBlock()
    {
        for (int i = 0; i < kTableSize; i++)
        {
            if (fat_table_in_memory[i] == -2)
            {
                return i + bufferOffset; // Returning the starting block
            }
        }
        return -1;
    }

    /* This method is used to get the fat chain of a given file by using the starting block,
    This method returns a vector with the numbers of the chain
     */
    std::vector<int> get_fat_chain(int starting_block, std::array<int, kTableSize> fat_table)
    {
        std::vector<int> result;

        int next_block = starting_block;

        while (next_block != -1)
        {
            result.emplace_back(next_block);                   // Pushing back to the vector
            next_block = fat_table[next_block - bufferOffset]; // Traversing through the fat table
        }
        return result;
    }

    /* This method is used to print out the fat table
     */
    Status printFatTable()
    {
        for (int i = 0; i < kTableSize; i++)
        {
            cout << i << ":[" << fat_table_in_memory[i] << "]" << endl;
        }
        return Status::Ok;
    }
};

/*This method is used to create the linux menu and is an infite loop. To exit type exit
This method has no return type.*/
void menu()
{
    FATFilesystem a;
    string choice = "";
    Status s;
    // a.createFile("Test.txt", 40, b);
    s = a.listDirectory();
    if (s == Status::Ok)
        while (choice != "exit")
        {
            std::string acc = "";
            getline(cin, acc);
            if (acc == "")
            {
                cout << "Wrong input, Try again" << endl;
                continue;
            }
            std::stringstream ss(acc);
            string b;
            vector<string> arr;

            while (getline(ss, b, ' '))
            {
                // store token string in the vector
                arr.push_back(b);
            }

            if (arr.at(0) == "ls")
            {
                s = a.listDirectory();
            }

            else if (arr.at(0) == "mv")
            {
                s = a.rename(arr.at(1), arr.at(2));
            }

            else if (arr.at(0) == "du")
            {
                s = a.diskUsage();
            }
            else if (arr.at(0) == "create")
            {

                auto d = stoi(arr.at(2));
                string byteString = arr.at(3);
                char aa = byteString[0];
                std::byte bt = (std::byte)aa;
                s = a.createFile(arr.at(1), d, bt);
            }
            else if (arr.at(0) == "cat")
            {
                s = a.print(arr.at(1));
            }
            else if (arr.at(0) == "rm")
            {
                s = a.remove(arr.at(1));
            }
            else if (arr.at(0) == "fat")
            {
                s = a.printFatTable();
            }
            else if (arr.at(0) == "exit")
            {
                exit(0);
            }
            else
                cout << "No command found" << endl;
        }
}

int main()
{

    std::cout << sizeof(std::array<FileEntry, kMaxNumberFiles>) << " bytes" << std::endl;
    std::cout << sizeof(std::array<int, kTableSize>) << " bytes" << std::endl;

    // TODO - CONSOLE HERE
    menu();

    return 0;
}