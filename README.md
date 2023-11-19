# FAT_filesystem


Developed a 32-bit FAT file system in CPP with a fully function command line interface, and 1024 bits of storage. Files contain byte data and use a FAT multi-block storage theme. ![image](https://github.com/Deniz-Misirlioglu/FAT_filesystem/assets/104803792/650d1f7b-dc1b-4521-9987-011459e309cd)

All functionality of this project includes

FATFilesystem(); 
Constructs the struct with an array of fixed size and initializes the file allocation table.

Status createFile(const std::string& filename unsigned int filesize,std::byte value)
Creates a file ‘filename’ with size ‘filesize’. The file is filled with the value ‘value’. 
Prints on success:Success: File ‘filename’ with size ‘filesize’ and value‘value’ has been created
Prints ‘Error: string has non-alphabetic characters’ when there is a non-alphabetic character present.
Prints ‘Error: Array is full’ and returns Status::Error when the array is full.
Prints ‘Error: Max files reached’ and returns
Status::Error when there is no more space for the file in the root directory.

Status listDirectory(); Prints the contents of the directory with sizes and dates (this should print the root directory contents).
Prints ‘Error: something went wrong’ and returns
Status::Error when an error occurs.

Status remove(const std::string& filename)
Deletes the file ‘filename’ from the drive.
Prints ‘Error: file does not exist’ and returns
Status::Error when the file does not exist.

Status rename(const std::string& filename, const std::string& new_filename)
Renames the file from ‘filename’ to ‘new_filename’
Prints ‘Error: string has non-alphabetic characters’ when there is a non-alphabetic character present in either filenames.
Prints ‘Error: file does not exist’ and returns
Status::Error when the file does not exist.

Status print(conststd::string& filename)
Prints the contents of file ‘filename’.
Prints “Error: file does not exist and returns Status::Error
