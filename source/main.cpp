#include <iostream>

#include "fatfs.h"

int main(int, char**)
{
    FATFileSystem fs("fatdisk.img");

    return 0;
}