#pragma once

#include "../../vendor/ooz/ooz.hpp"

#include <string>
#include <vector>
#include <filesystem>

#define SAFE_SPACE 64

namespace fs = std::filesystem;

namespace HAYDEN
{
    // Decompress using ooz lib
    std::vector<uint8_t> oodleDecompress(std::vector<uint8_t> compressedData, const uint64_t decompressedSize);
}