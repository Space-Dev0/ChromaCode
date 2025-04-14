#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include "./encoder.hpp"

#define VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define reportError(a) std::cout<<a<<'\n';exit(15);

std::string data;
std::string filename;
int32_t color_number=0;
int32_t symbol_number=0;
int32_t module_size=0;
int32_t master_symbol_width=0;
int32_t master_symbol_height=0;
int32_t symbol_positions=0;
int32_t symbol_positions_number=0;
vector2d* symbol_versions=0;
int32_t symbol_versions_number=0;
std::vector<int32_t> symbol_ecc_levels;
int32_t symbol_ecc_levels_number=0;
int32_t color_space=0;

void printUsage()
{
  std::cout<<'\n';
  std::cout<<"ChromaCodeWriter (Version "<<VERSION <<" Build date: "<<BUILD_DATE<<")";
  std::cout<<"Usage:\n\n";
  std::cout<<"ChromaCodeWriter --input message to encode --out output-image [options]\n\n";
  std::cout<<"--input\t\t\tInput data(message to be encoded).\n";
  std::cout<<"--input-file\t\tInput data file.\n";
  std::cout<<"--output\t\tOutput image file.\n";
  std::cout<<"--color-number\t\tNumber of colors (4, 8, default: 8).\n";
  std::cout<<"--module-size\t\tModule size in pixel (default: 12 pixels).\n";
  std::cout<<"--symbol-width\t\tMaster symbol width in pixel.\n";
  std::cout<<"--symbol-height\t\tMaster symbol height in pixel.\n";
  std::cout<<"--symbol-number\t\tNumber of symbols (1-61, default: 1).\n";
  std::cout<<"--ecc-level\t\tError correction levels (1-10, default: 3(6%)) If\n\t\t\tdifferent for each symbol, starting from master and\n\t\t\tthen slave symbols (ecc0 ecc1 ecc2...). For master\n\t\t\tsymbol, level 0 means using the default level, for\n\t\t\tslaves, it means using same level as its host.\n";
  std::cout<<"--symbol-version\tSide version of each symbol, starting from master and \n\t\t\tthen slave symbols (x0 y0 x1 y1 x2 y2...).\n";
  std::cout<<"--symbol-position\tSymbol positions (0-60), starting from master and\n\t\t\tmulti-symbol code.\n";
  std::cout<<"--color-space\t\tColor space of output image (0: RGB, 1: CMYK, default: 0).\n\t\t\tRGB image is saved as PNG and CMYK as TIFF.\n";
  std::cout<<"--help\t\t\tPrint this help.\n\n";
  std::cout<<"Example for 1-symbol-code: \n";
  std::cout<<"ChromaCodeWriter --input 'Hello world' --output test.png\n\n";
  std::cout<<"Example for 3-symbol-code: \n";
  std::cout<<"ChromaCodeWriter --input 'Hello world' --output test.png --symbol-number 3 --symbol-position 0 3 2 --sybol-version 3 2 4 2 3 2\n\n";
}

bool parseCommandLineParameters(int32_t para_number, char* para[])
{
  for (int32_t loop = 1; loop<para_number; loop++) {
  if (0==strcmp(para[loop],"--input")){
    if (loop + 1 > para_number - 1) {
      std::cout<<"Value for option '"<<para[loop]<<"' missing.\n";
      return false;
    }
    std::string data_string = para[++loop];
    memcpy(&data, &data_string, data_string.length());
  }
  else if (0==strcmp(para[loop],"--input-file")) {
    if (loop+1>para_number-1) {
      std::cout<<"value for option '"<<para[loop]<<"' missing.\n";
      return false;
    }
    FILE *fp = fopen(para[++loop],"rb");
    if (!fp) {
      reportError("Opening input data file failed.");
    }
  }
  }
}

