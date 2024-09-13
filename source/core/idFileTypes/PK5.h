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
    struct PK5_HEADER
    {
        uint32_t Magic = 0;
        uint64_t IndexStart = 0;
        uint64_t IndexSize = 0;
    };

    struct PK5_ENTRY {
        uint64_t DataStartOffset = 0;
        uint32_t UncompressedSize = 0;
        uint32_t CompressedSize = 0;
        uint32_t CompressionMode = 0; // 11 is kraken
        uint32_t Timestamp = 0;
    };

    class PK5
    {
        public:
            std::string FilePath;

            PK5_HEADER Header;
            uint32_t EntryCount;
            std::vector<PK5_ENTRY> FileEntries;
            std::vector<std::string> EntryNames;
            std::vector<int> EntryVersions;
            std::vector<EntryType> EntryTypes;
            std::vector<std::string> EmbeddedTypes;
            PK5(const fs::path filePath);
    };
}

#pragma pack(pop)