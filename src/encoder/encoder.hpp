#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <cstdint>
#include <iostream>
#include <vector>
#include <cmath>
#include "ldpc.hpp"
#include "pseudoRandom.hpp"
#include "./include/png.h"

#define DEFAULT_SYMBOL_NUMBER 1
#define DEFAULT_MODULE_SIZE 12
#define DEFAULT_COLOR_NUMBER 8
#define DEFAULT_MODULE_COLOR_MODE 2
#define DEFAULT_ECC_LEVEL 3
#define DEFAULT_MASKING_REFERENCE 7

#define MAX_SYMBOL_NUMBER 61
#define MAX_COLOR_NUMBER 256
#define MAX_SIZE_ENCODING_MODE 256
#define ENCODING_MODES 6
#define ENC_MAX 1000000
#define NUMBER_OF_MASK_PATTERNS 8

#define MASTER_METADATA_PART1_LENGTH 6
#define MASTER_METADATA_PART2_LENGTH 38
#define MASTER_METADATA_PART1_MODULE_NUMBER 4

#define DISTANCE_TO_BORDER 4
#define MAX_ALIGNMENT_NUMBER 9
#define COLOR_PALETTE_NUMBER 4

#define MASTER_METADATA_X 6
#define MASTER_METADATA_Y 1

#define FP0_CORE_COLOR 0
#define FP1_CORE_COLOR 0
#define FP2_CORE_COLOR 6
#define FP3_CORE_COLOR 3

#define AP0_CORE_COLOR 3
#define AP1_CORE_COLOR 3
#define AP2_CORE_COLOR 3
#define AP3_CORE_COLOR 3
#define APX_CORE_COLOR 6

#define BITMAP_BITS_PER_PIXEL 32
#define BITMAP_BITS_PER_CHANNEL 8
#define BITMAP_CHANNEL_COUNT 4

#define INTERLEAVE_SEED 226759

#define W1 100
#define W2 3
#define W3 3

#define VERSION2SIZE(x) (x * 4 + 17)
#define SIZE2VERSION(x) ((x - 17) / 4)
#define MAX(a, b) ({__typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({__typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

struct rgb
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    constexpr rgb(uint8_t a, uint8_t b, uint8_t c) : R(a), G(b), B(c) {}
};

struct vector2d
{
    int x;
    int y;
};

struct bitmap
{
    int width;
    int height;
    int bits_per_pixel;
    int bits_per_channel;
    int channel_count;
    std::vector<uint8_t> pixel;
};

constexpr rgb black(0, 0, 0);
constexpr rgb blue(0, 0, 255);
constexpr rgb green(0, 255, 0);
constexpr rgb cyan(0, 255, 255);
constexpr rgb red(255, 0, 0);
constexpr rgb magenta(255, 0, 255);
constexpr rgb yellow(255, 255, 0);
constexpr rgb white(255, 255, 255);

constexpr int master_palette_placement_index[4][8] =
    {{0, 3, 5, 6, 1, 2, 4, 7}, {0, 6, 5, 3, 1, 2, 4, 7}, {6, 0, 5, 3, 1, 2, 4, 7}, {3, 0, 5, 6, 1, 2, 4, 7}};

constexpr int slave_palette_placement_index[8] = {3, 6, 5, 0, 1, 2, 4, 7};

constexpr vector2d slave_palette_position[32] = {
    {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {4, 10}, {4, 11}, {4, 12}, {5, 12}, {5, 11}, {5, 10}, {5, 9}, {5, 8}, {5, 7}, {5, 6}, {5, 5}, {6, 5}, {6, 6}, {6, 7}, {6, 8}, {6, 9}, {6, 10}, {6, 11}, {6, 12}, {7, 12}, {7, 11}, {7, 10}, {7, 9}, {7, 8}, {7, 7}, {7, 6}, {7, 5}};

constexpr int modeSwitch[7][16] =
    {{-1, 28, 29, -1, -1, 30, -1, -1, -1, -1, 27, 125, -1, 124, 126, -1},
     {126, -1, 29, -1, -1, 30, -1, 28, -1, 127, 27, 125, -1, 124, -1, 127},
     {14, 63, -1, -1, -1, 478, -1, 62, -1, -1, 13, 61, -1, 60, -1, -1},
     {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
     {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
     {255, 8188, 8189, -1, -1, -1, -1, -1, -1, -1, 254, 253, -1, 252, -1, -1},
     {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

constexpr int characterSize[] = {5, 5, 4, 4, 5, 6, 8};

constexpr uint8_t fp0_core_color_index[] = {0, 0, FP0_CORE_COLOR, 0, 0, 0, 0, 0};
constexpr uint8_t fp1_core_color_index[] = {0, 0, FP1_CORE_COLOR, 0, 0, 0, 0, 0};
constexpr uint8_t fp2_core_color_index[] = {0, 2, FP2_CORE_COLOR, 14, 30, 60, 124, 252};
constexpr uint8_t fp3_core_color_index[] = {0, 3, FP3_CORE_COLOR, 3, 7, 15, 15, 31};

constexpr uint8_t apn_core_color_index[] = {0, 3, AP0_CORE_COLOR, 3, 7, 15, 15, 31};
constexpr uint8_t apx_core_color_index[] = {0, 2, APX_CORE_COLOR, 14, 30, 60, 124, 252};

constexpr int latchShiftTo[14][14] =
    {{0, 5, 5, ENC_MAX, ENC_MAX, 5, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 5, 7, ENC_MAX, 11},
     {7, 0, 5, ENC_MAX, ENC_MAX, 5, ENC_MAX, 5, ENC_MAX, ENC_MAX, 5, 7, ENC_MAX, 11},
     {4, 6, 0, ENC_MAX, ENC_MAX, 9, ENC_MAX, 6, ENC_MAX, ENC_MAX, 4, 6, ENC_MAX, 10},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, ENC_MAX, ENC_MAX, 0, ENC_MAX},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, ENC_MAX, ENC_MAX, 0, ENC_MAX},
     {8, 13, 13, ENC_MAX, ENC_MAX, 0, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 8, 8, ENC_MAX, 12},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, 0, ENC_MAX, ENC_MAX, 0, 0},
     {0, 5, 5, ENC_MAX, ENC_MAX, 5, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 5, 7, ENC_MAX, 11},
     {7, 0, 5, ENC_MAX, ENC_MAX, 5, ENC_MAX, 5, ENC_MAX, ENC_MAX, 5, 7, ENC_MAX, 11},
     {4, 6, 0, ENC_MAX, ENC_MAX, 9, ENC_MAX, 6, ENC_MAX, ENC_MAX, 4, 6, ENC_MAX, 10},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, ENC_MAX, ENC_MAX, 0, ENC_MAX},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, ENC_MAX, ENC_MAX, 0, ENC_MAX},
     {8, 13, 13, ENC_MAX, ENC_MAX, 0, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 8, 8, ENC_MAX, 12},
     {ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, ENC_MAX, 0, 0, 0, 0, ENC_MAX, ENC_MAX, 0, 0}}; // First latch then shift

constexpr vector2d symbolPos[MAX_SYMBOL_NUMBER] =
    {{0, 0},
     {0, -1},
     {0, 1},
     {-1, 0},
     {1, 0},
     {0, -2},
     {-1, -1},
     {1, -1},
     {0, 2},
     {-1, 1},
     {1, 1},
     {-2, 0},
     {2, 0},
     {0, -3},
     {-1, -2},
     {1, -2},
     {-2, -1},
     {2, -1},
     {0, 3},
     {-1, 2},
     {1, 2},
     {-2, 1},
     {2, 1},
     {-3, 0},
     {3, 0},
     {0, -4},
     {-1, -3},
     {1, -3},
     {-2, -2},
     {2, -2},
     {-3, -1},
     {3, -1},
     {0, 4},
     {-1, 3},
     {1, 3},
     {-2, 2},
     {2, 2},
     {-3, 1},
     {3, 1},
     {-4, 0},
     {4, 0},
     {0, -5},
     {-1, -4},
     {1, -4},
     {-2, -3},
     {2, -3},
     {-3, -2},
     {3, -2},
     {-4, -1},
     {4, -1},
     {0, 5},
     {-1, 4},
     {1, 4},
     {-2, 3},
     {2, 3},
     {-3, 2},
     {3, 2},
     {-4, 1},
     {4, 1},
     {-5, 0},
     {5, 0}};

constexpr uint8_t nc_color_encode_table[8][2] = {{0, 0}, {0, 3}, {0, 6}, {3, 0}, {3, 3}, {3, 6}, {6, 0}, {6, 3}};

constexpr int encodingTable[MAX_SIZE_ENCODING_MODE][ENCODING_MODES] =
    {
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 16, -1},
        {-1, -1, -1, -1, 17, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -19, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {0, 0, 0, -1, -1, 0},
        {-1, -1, -1, 0, -1, -1},
        {-1, -1, -1, 1, -1, -1},
        {-1, -1, -1, -1, 0, -1},
        {-1, -1, -1, 2, -1, -1},
        {-1, -1, -1, 3, -1, -1},
        {-1, -1, -1, 4, -1, -1},
        {-1, -1, -1, 5, -1, -1},
        {-1, -1, -1, 6, -1, -1},
        {-1, -1, -1, 7, -1, -1},
        {-1, -1, -1, -1, 1, -1},
        {-1, -1, -1, -1, 2, -1},
        {-1, -1, 11, 8, -20, -1},
        {-1, -1, -1, 9, -1, -1},
        {-1, -1, 12, 10, -21, -1},
        {-1, -1, -1, 11, -1, -1},
        {-1, -1, 1, -1, -1, 1},
        {-1, -1, 2, -1, -1, 2},
        {-1, -1, 3, -1, -1, 3},
        {-1, -1, 4, -1, -1, 4},
        {-1, -1, 5, -1, -1, 5},
        {-1, -1, 6, -1, -1, 6},
        {-1, -1, 7, -1, -1, 7},
        {-1, -1, 8, -1, -1, 8},
        {-1, -1, 9, -1, -1, 9},
        {-1, -1, 10, -1, -1, 10},
        {-1, -1, -1, 12, -22, -1},
        {-1, -1, -1, 13, -1, -1},
        {-1, -1, -1, -1, 3, -1},
        {-1, -1, -1, -1, 4, -1},
        {-1, -1, -1, -1, 5, -1},
        {-1, -1, -1, 14, -1, -1},
        {-1, -1, -1, 15, -1, -1},
        {1, -1, -1, -1, -1, 11},
        {2, -1, -1, -1, -1, 12},
        {3, -1, -1, -1, -1, 13},
        {4, -1, -1, -1, -1, 14},
        {5, -1, -1, -1, -1, 15},
        {6, -1, -1, -1, -1, 16},
        {7, -1, -1, -1, -1, 17},
        {8, -1, -1, -1, -1, 18},
        {9, -1, -1, -1, -1, 19},
        {10, -1, -1, -1, -1, 20},
        {11, -1, -1, -1, -1, 21},
        {12, -1, -1, -1, -1, 22},
        {13, -1, -1, -1, -1, 23},
        {14, -1, -1, -1, -1, 24},
        {15, -1, -1, -1, -1, 25},
        {16, -1, -1, -1, -1, 26},
        {17, -1, -1, -1, -1, 27},
        {18, -1, -1, -1, -1, 28},
        {19, -1, -1, -1, -1, 29},
        {20, -1, -1, -1, -1, 30},
        {21, -1, -1, -1, -1, 31},
        {22, -1, -1, -1, -1, 32},
        {23, -1, -1, -1, -1, 33},
        {24, -1, -1, -1, -1, 34},
        {25, -1, -1, -1, -1, 35},
        {26, -1, -1, -1, -1, 36},
        {-1, -1, -1, -1, 6, -1},
        {-1, -1, -1, -1, 7, -1},
        {-1, -1, -1, -1, 8, -1},
        {-1, -1, -1, -1, 9, -1},
        {-1, -1, -1, -1, 10, -1},
        {-1, -1, -1, -1, 11, -1},
        {-1, 1, -1, -1, -1, 37},
        {-1, 2, -1, -1, -1, 38},
        {-1, 3, -1, -1, -1, 39},
        {-1, 4, -1, -1, -1, 40},
        {-1, 5, -1, -1, -1, 41},
        {-1, 6, -1, -1, -1, 42},
        {-1, 7, -1, -1, -1, 43},
        {-1, 8, -1, -1, -1, 44},
        {-1, 9, -1, -1, -1, 45},
        {-1, 10, -1, -1, -1, 46},
        {-1, 11, -1, -1, -1, 47},
        {-1, 12, -1, -1, -1, 48},
        {-1, 13, -1, -1, -1, 49},
        {-1, 14, -1, -1, -1, 50},
        {-1, 15, -1, -1, -1, 51},
        {-1, 16, -1, -1, -1, 52},
        {-1, 17, -1, -1, -1, 53},
        {-1, 18, -1, -1, -1, 54},
        {-1, 19, -1, -1, -1, 55},
        {-1, 20, -1, -1, -1, 56},
        {-1, 21, -1, -1, -1, 57},
        {-1, 22, -1, -1, -1, 58},
        {-1, 23, -1, -1, -1, 59},
        {-1, 24, -1, -1, -1, 60},
        {-1, 25, -1, -1, -1, 61},
        {-1, 26, -1, -1, -1, 62},
        {-1, -1, -1, -1, 12, -1},
        {-1, -1, -1, -1, 13, -1},
        {-1, -1, -1, -1, 14, -1},
        {-1, -1, -1, -1, 15, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 23, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 24, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 25, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 26, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 27, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 28, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 29, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 30, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, 31, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1},
        {-1, -1, -1, -1, -1, -1}};

constexpr int apNum[32] = {2, 2, 2, 2, 2,
                           3, 3, 3, 3,
                           4, 4, 4, 4,
                           5, 5, 5, 5,
                           6, 6, 6, 6,
                           7, 7, 7, 7,
                           8, 8, 8, 8,
                           9, 9, 9};

constexpr int apPos[32][9] = {{4, 18, 0, 0, 0, 0, 0, 0, 0},
                              {4, 22, 0, 0, 0, 0, 0, 0, 0},
                              {4, 26, 0, 0, 0, 0, 0, 0, 0},
                              {4, 30, 0, 0, 0, 0, 0, 0, 0},
                              {4, 34, 0, 0, 0, 0, 0, 0, 0},
                              {4, 17, 38, 0, 0, 0, 0, 0, 0},
                              {4, 20, 42, 0, 0, 0, 0, 0, 0},
                              {4, 23, 46, 0, 0, 0, 0, 0, 0},
                              {4, 26, 50, 0, 0, 0, 0, 0, 0},
                              {4, 14, 32, 54, 0, 0, 0, 0, 0},
                              {4, 17, 39, 58, 0, 0, 0, 0, 0},
                              {4, 20, 46, 62, 0, 0, 0, 0, 0},
                              {4, 23, 44, 66, 0, 0, 0, 0, 0},
                              {4, 26, 37, 51, 70, 0, 0, 0, 0},
                              {4, 14, 36, 58, 74, 0, 0, 0, 0},
                              {4, 17, 39, 56, 78, 0, 0, 0, 0},
                              {4, 20, 42, 63, 82, 0, 0, 0, 0},
                              {4, 23, 38, 54, 70, 86, 0, 0, 0},
                              {4, 26, 38, 56, 77, 90, 0, 0, 0},
                              {4, 14, 33, 53, 72, 94, 0, 0, 0},
                              {4, 17, 38, 59, 79, 98, 0, 0, 0},
                              {4, 20, 36, 53, 70, 86, 102, 0, 0},
                              {4, 23, 36, 55, 74, 93, 106, 0, 0},
                              {4, 26, 36, 58, 79, 100, 110, 0, 0},
                              {4, 14, 36, 58, 80, 92, 114, 0, 0},
                              {4, 17, 34, 52, 70, 88, 99, 118, 0},
                              {4, 20, 37, 54, 72, 89, 106, 122, 0},
                              {4, 23, 38, 56, 74, 92, 113, 126, 0},
                              {4, 26, 36, 58, 78, 98, 120, 130, 0},
                              {4, 14, 32, 49, 67, 84, 102, 112, 134},
                              {4, 17, 35, 53, 71, 89, 107, 119, 138},
                              {4, 20, 38, 55, 73, 91, 108, 126, 142}};

constexpr float ecclevel2coderate[11] = {0.55f, 0.63f, 0.57f, 0.55f, 0.50f, 0.43f, 0.34f, 0.25f, 0.20f, 0.17f, 0.14f};

constexpr int ecclevel2wcwr[11][2] = {{4, 9}, {3, 8}, {3, 7}, {4, 9}, {3, 6}, {4, 7}, {4, 6}, {3, 4}, {4, 5}, {5, 6}, {6, 7}};

struct chromaCode
{
    int dimension;      ///< Module size in pixel
    vector2d code_size; ///< Code size in symbol
    int min_x;
    int min_y;
    int rows;
    int cols;
    std::vector<int> row_height;
    std::vector<int> col_width;
};

class encode;

class symbol
{
    friend class encode;
    friend int getSymbolCapacity(encode &enc, int index);

public:
    int index;
    vector2d side_size;
    int host;
    int slaves[4];
    int wcwr[2];
    std::string data;
    std::vector<uint8_t> dataMap;
    std::string metadata;
    std::vector<uint8_t> matrix;
    void getOptimalECC(int capacity, int net_data_length);
};

class encode
{
    friend class symbol;

private:
    int maskCode();
    void maskSymbols(int mask_type, int *masked);
    void setDefaultPalette(int color_number, std::vector<rgb> &palette);
    void setDefaultEccLevels(int symbol_number, std::vector<uint8_t> &ecc_levels);
    void swap_byte(uint8_t *a, uint8_t *b);
    void swap_int(int *a, int *b);
    std::vector<int> encodeSeq;
    bool isDefaultMode();
    void getNextMetadataModuleInMaster(int matrix_height, int matrix_width, int next_module_count, int *x, int *y);
    void convert_dec_to_bin(int dec, std::string &bin, int start_position, int length);
    void swap_symbols(int index1, int index2);
    int getMetadataLength(int index);
    int getSymbolCapacity(int index);
    bool createMatrix(int index, std::string &ecc_encoded_data);
    std::string encodedData;
    int encodedLength;
    chromaCode p;

public:
    int colorNumber;
    int symbolNumber;
    int moduleSize{DEFAULT_MODULE_SIZE};
    int masterSymbolWidth{0};
    int masterSymbolHeight{0};
    std::vector<rgb> palette;
    std::vector<vector2d> symbolVersions;
    std::vector<uint8_t> symbolEccLevels;
    std::vector<int> symbolPositions;
    std::vector<symbol> symbols;
    bitmap bitmp;
    std::vector<int> analyzeInputData(std::string &input);
    bool encodeMasterMetadata();
    bool updateMasterMetadataPartII(int mask_ref);
    void placeMasterMetadataPartII();
    void encodeData(std::string &data);
    bool assignDockedSymbols();
    void getCodePara();
    bool createBitmap();
    bool checkDockedSymbolSize();
    bool setMasterSymbolVersion();
    bool addE2SlaveMetadata(symbol &slave);
    void updateSlaveMetadataE(int host_index, int slave_index);
    bool fitDataIntoSymbols();
    bool InitSymbols();
    bool setSlaveMetadata();
    int generateChromaCode(std::string &data);
    encode(int color_number, int symbol_number);
    ~encode();
};

#endif