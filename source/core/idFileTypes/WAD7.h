#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "ResourceFile.h"
#include "../Utilities.h"

#pragma pack(push)  // Not portable, sorry.
#pragma pack(1)     // Works on my machine (TM).

namespace fs = std::filesystem;

namespace HAYDEN
{
    struct WAD7_HEADER
    {
        uint32_t Magic = 0;
        uint16_t Version = 0;
        uint64_t Checksum = 0;
        char     Padding[5] = { 0, 0, 0, 0, 0 };
        uint64_t IndexStart = 0;
        uint64_t IndexSize = 0;
    };

    struct WAD7_ENTRY {
        uint64_t DataStartOffset = 0;
        uint32_t UncompressedSize = 0;
        uint32_t CompressedSize = 0;
        uint32_t CompressionMode = 0; // 11 is kraken
        uint32_t Unknown0 = 0;
        uint64_t Unknown1 = 0;
    };

    class WAD7
    {
        public:
            std::string FilePath;

            WAD7_HEADER Header;
            uint32_t EntryCount;
            std::vector<WAD7_ENTRY> FileEntries;
            std::vector<std::string> EntryNames;
            std::vector<int> EntryVersions;
            std::vector<EntryType> EntryTypes;
            std::vector<std::string> EmbeddedTypes;
            WAD7(const fs::path filePath);
    };
}

#pragma pack(pop)