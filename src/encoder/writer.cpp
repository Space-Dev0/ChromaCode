#include "./encoder.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#define VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define reportError(a) std::cout << a << '\n';

std::string data;
std::string filename;
int32_t color_number = 0;
int32_t symbol_number = 0;
int32_t module_size = 0;
int32_t master_symbol_width = 0;
int32_t master_symbol_height = 0;
std::vector<int32_t> symbol_positions;
int32_t symbol_positions_number = 0;
std::vector<vector2d> symbol_versions;
int32_t symbol_versions_number = 0;
std::vector<uint8_t> symbol_ecc_levels;
int32_t symbol_ecc_levels_number = 0;
int32_t color_space = 0;

void printUsage()
{
  std::cout << '\n';
  std::cout << "ChromaCodeWriter (Version " << VERSION
            << " Build date: " << BUILD_DATE << ")";
  std::cout << "Usage:\n\n";
  std::cout << "ChromaCodeWriter --input message to encode --out output-image "
               "[options]\n\n";
  std::cout << "--input\t\t\tInput data(message to be encoded).\n";
  std::cout << "--input-file\t\tInput data file.\n";
  std::cout << "--output\t\tOutput image file.\n";
  std::cout << "--color-number\t\tNumber of colors (4, 8, default: 8).\n";
  std::cout << "--module-size\t\tModule size in pixel (default: 12 pixels).\n";
  std::cout << "--symbol-width\t\tMaster symbol width in pixel.\n";
  std::cout << "--symbol-height\t\tMaster symbol height in pixel.\n";
  std::cout << "--symbol-number\t\tNumber of symbols (1-61, default: 1).\n";
  std::cout << "--ecc-level\t\tError correction levels (1-10, default: 3(6%)) "
               "If\n\t\t\tdifferent for each symbol, starting from master "
               "and\n\t\t\tthen slave symbols (ecc0 ecc1 ecc2...). For "
               "master\n\t\t\tsymbol, level 0 means using the default level, "
               "for\n\t\t\tslaves, it means using same level as its host.\n";
  std::cout
      << "--symbol-version\tSide version of each symbol, starting from master "
         "and \n\t\t\tthen slave symbols (x0 y0 x1 y1 x2 y2...).\n";
  std::cout << "--symbol-position\tSymbol positions (0-60), starting from "
               "master and\n\t\t\tmulti-symbol code.\n";
  std::cout
      << "--color-space\t\tColor space of output image (0: RGB, 1: CMYK, "
         "default: 0).\n\t\t\tRGB image is saved as PNG and CMYK as TIFF.\n";
  std::cout << "--help\t\t\tPrint this help.\n\n";
  std::cout << "Example for 1-symbol-code: \n";
  std::cout << "ChromaCodeWriter --input 'Hello world' --output test.png\n\n";
  std::cout << "Example for 3-symbol-code: \n";
  std::cout << "ChromaCodeWriter --input 'Hello world' --output test.png "
               "--symbol-number 3 --symbol-position 0 3 2 --sybol-version 3 2 "
               "4 2 3 2\n\n";
}

bool parseCommandLineParameters(int32_t para_number, char *para[])
{
  for (int32_t loop = 1; loop < para_number; loop++)
  {
    if (0 == strcmp(para[loop], "--input"))
    {
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << para[loop] << "' missing.\n";
        return false;
      }
      data = para[++loop];
    }
    else if (0 == strcmp(para[loop], "--input-file"))
    {
      if (loop + 1 > para_number - 1)
      {
        std::cout << "value for option '" << para[loop] << "' missing.\n";
        return false;
      }
      FILE *fp = fopen(para[++loop], "rb");
      if (!fp)
      {
        reportError("Opening input data file failed.");
        return false;
      }
      int32_t file_size;
      fseek(fp, 0, SEEK_END);
      file_size = ftell(fp);
      data.reserve(file_size);
      fseek(fp, 0, SEEK_SET);
      char *temp;
      if (fread(temp, 1, file_size, fp) != file_size)
      {
        reportError("Reading input data file failed");
        fclose(fp);
        return false;
      }
      data = temp;
      fclose(fp);
    }
    else if (0 == strcmp(para[loop], "--output"))
    {
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << para[loop] << "' missing.\n";
        return false;
      }
      filename = para[++loop];
    }
    else if (0 == strcmp(para[loop], "--color-number"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      color_number = strtol(para[++loop], &endptr, 10);
      if (*endptr)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
      if (color_number != 4 && color_number != 8)
      {
        reportError(
            "Invalid color number. Supported color number includes 4 and 8.");
        return false;
      }
    }
    else if (0 == strcmp(para[loop], "--module-size"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      module_size = strtol(para[++loop], &endptr, 10);
      if (*endptr || module_size < 0)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
    }
    else if (0 == strcmp(para[loop], "--symbol-width"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      master_symbol_width = strtol(para[++loop], &endptr, 10);
      if (*endptr || master_symbol_width < 0)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
    }
    else if (0 == strcmp(para[loop], "--symbol-height"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      master_symbol_height = strtol(para[++loop], &endptr, 10);
      if (*endptr || master_symbol_height < 0)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
    }
    else if (0 == strcmp(para[loop], "--symbol-number"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      symbol_number = strtol(para[++loop], &endptr, 10);
      if (*endptr)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
      if (symbol_number < 1 || symbol_number > MAX_SYMBOL_NUMBER)
      {
        reportError("Invalid symbol number (must be 1 - 61).");
        return false;
      }
    }
    else if (0 == strcmp(para[loop], "--color-space"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      char *endptr;
      color_space = strtol(para[++loop], &endptr, 10);
      if (*endptr)
      {
        std::cout << "Invalid or missing values for option '" << option
                  << "'.\n";
        return false;
      }
      if (color_space != 0 && color_space != 1)
      {
        reportError("Invalid color space (must be 0 or 1).");
        return false;
      }
    }
  }

  // check input
  if (data.length() == 0)
  {
    reportError("Input data is empty");
    return false;
  }
  if (filename.empty())
  {
    reportError("Output file missing");
    return false;
  }
  if (symbol_number == 0)
  {
    symbol_number = 1;
  }

  // second scan
  for (int32_t loop = 1; loop < para_number; loop++)
  {
    if (0 == strcmp(para[loop], "--ecc-level"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      symbol_ecc_levels.reserve(symbol_number);
      for (int32_t j = 0; j < symbol_number; j++)
      {
        if (loop + 1 > para_number - 1)
          break;
        char *endptr;
        symbol_ecc_levels[j] = strtol(para[++loop], &endptr, 10);
        if (*endptr)
        {
          if (symbol_ecc_levels_number == 0)
          {
            std::cout << "Value for option '" << option
                      << "' missing or invalid.\n";
            return false;
          }
          loop--;
          break;
        }
        if (symbol_ecc_levels[j] < 0 || symbol_ecc_levels[j] > 10)
        {
          reportError("Invalid error correction level (must be 1 - 10).");
          return false;
        }
        symbol_ecc_levels_number++;
      }
    }
    else if (0 == strcmp(para[loop], "--symbol-version"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      symbol_versions.reserve(symbol_number);
      for (int32_t j = 0; j < symbol_number; j++)
      {
        if (loop + 1 > para_number - 1)
        {
          std::cout << "Too few values for option '" << option << "'.\n";
          return false;
        }
        char *endptr;
        symbol_versions[j].x = strtol(para[++loop], &endptr, 10);
        if (*endptr)
        {
          std::cout << "Invalid or missing values for option '" << option
                    << "'.\n";
          return false;
        }
        if (loop + 1 > para_number - 1)
        {
          std::cout << "Too few values for option '" << option << "'.\n";
          return false;
        }
        symbol_versions[j].y = strtol(para[++loop], &endptr, 10);
        if (*endptr)
        {
          std::cout << "Invalid or missing values for option '" << option
                    << "'.\n";
          return false;
        }
        if (symbol_versions[j].x < 1 || symbol_versions[j].x > 32 ||
            symbol_versions[j].y < 1 || symbol_versions[j].y > 32)
        {
          reportError("Invalid symbol side version (must be 1 - 32).");
          return false;
        }
        symbol_versions_number++;
      }
    }
    else if (0 == strcmp(para[loop], "--symbol-position"))
    {
      char *option = para[loop];
      if (loop + 1 > para_number - 1)
      {
        std::cout << "Value for option '" << option << "' missing.\n";
        return false;
      }
      symbol_positions.reserve(symbol_number);
      for (int32_t j = 0; j < symbol_number; j++)
      {
        if (loop + 1 > para_number - 1)
        {
          std::cout << "Too few values for option '" << option << "'.\n";
          return false;
        }
        char *endptr;
        symbol_positions[j] = strtol(para[++loop], &endptr, 10);
        if (*endptr)
        {
          std::cout << "Invalid or missing values for option '" << option
                    << "'.\n";
          return false;
        }
        if (symbol_positions[j] < 0 || symbol_positions[j] > 60)
        {
          reportError("Invalid symbol position value (must be 0 - 60).");
          return false;
        }
        symbol_positions_number++;
      }
    }
  }

  // check input
  if (symbol_number == 1 && symbol_positions.data())
  {
    if (symbol_positions[0] != 0)
    {
      reportError("Incorrect symbol position value for master symbol.");
      return false;
    }
  }
  if (symbol_number > 1 && symbol_positions_number != symbol_number)
  {
    reportError(
        "Symbol position information is incomplete for multi-symbol code");
    return false;
  }
  if (symbol_number > 1 && symbol_versions_number != symbol_number)
  {
    reportError(
        "Symbol version information is incomplete for multi-symbol code");
    return false;
  }
  return true;
}

int main() //(int argc, char *argv[])
{
  int argc = 5;
  char *argv[] = {"writer", "--input", "Fuck me", "--output", "test.png"};

  if (argc < 2 || (0 == strcmp(argv[1], "--help")))
  {
    printUsage();
    return 1;
  }
  if (!parseCommandLineParameters(argc, argv))
  {
    return 1;
  }

  encode enc(color_number, symbol_number);
  if (module_size > 0)
    enc.moduleSize = module_size;
  if (master_symbol_width > 0)
    enc.masterSymbolWidth = master_symbol_width;
  if (master_symbol_height > 0)
    enc.masterSymbolHeight = master_symbol_height;
  if (!symbol_ecc_levels.empty())
  {
    enc.symbolEccLevels = symbol_ecc_levels;
  }
  if (!symbol_versions.empty())
  {
    enc.symbolVersions = symbol_versions;
  }
  if (!symbol_positions.empty())
  {
    enc.symbolPositions = symbol_positions;
  }
  if (enc.generateChromaCode(data) != 0)
  {
    reportError("Creating chroma code failed.");
    return 1;
  }

  int32_t result = 0;
  if (color_space == 0)
  {
  }
}
