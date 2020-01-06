#include <iostream>

#include "fatfs.h"

int main(int, char**)
{
    FATFileSystem fs("fatdisk.img");

    fs.read_directory(2);
    return 0;
}