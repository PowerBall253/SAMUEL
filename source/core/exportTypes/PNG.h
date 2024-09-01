#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <filesystem>

#include "../Utilities.h"
#include "../exportTypes/DDSHeader.h"

#ifdef _WIN32
#include <wincodec.h>
#endif

namespace fs = std::filesystem;

namespace HAYDEN
{
    class PNGFile
    {
        public:
            bool ConvertDDStoPNG(std::vector<uint8_t> inputDDS, ImageType imageType, fs::path exportPath, bool reconstructZ = false);
    };
}