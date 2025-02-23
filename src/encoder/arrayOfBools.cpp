//
// Created by moonshot on 2/22/25.
//
#include "iostream"
#include "arrayOfBools.h"

arrayOfBools::arrayOfBools()
{
    for (int i = 0; i < 256; i++)
    {
        int k = i;
        for (int j = 7; j >= 0; j--)
        {
            if (k & 1)
                arr[i][j] = true;
            else
                arr[i][j] = false;
            k >>= 1;
        }
    }
}