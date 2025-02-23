/*
 * The following program follows ISO/IEC 23634:2022
 * Created by moonshot on 21/2/2025
 */
#include <iostream>
#include <fstream>
#include "arrayOfBools.h"
int main()
{
    std::ifstream ifile;
    ifile.open("../src/encoder/input.txt");
    if (!ifile.is_open()) {
        // Print an error message if the file couldn't be
        // opened
        std::cerr << "Error opening file!" << std::endl;
        // Return 1 to indicate failure
        return 1;
    }
    char ch;
    while (ifile.get(ch))
    {
        bool buf[24];
        int index = 0;
        const arrayOfBools A;
        std::cout << ch << "-";
        const bool* binary = A.arr[static_cast<unsigned char>(ch)];
        for (int i = index, j = 0; i < 8; i++)
        {
            buf[i] = binary[i];

        }
    }

}
