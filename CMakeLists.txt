cmake_minimum_required(VERSION 3.30)
project(Chroma)

set(CMAKE_CXX_STANDARD 23)

add_executable(Chroma src/encoder/main.cpp
        src/encoder/arrayOfBools.cpp
        src/encoder/arrayOfBools.h
)
