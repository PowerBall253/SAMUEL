#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "idFileTypes/ResourceFile.h"
#include "idFileTypes/PK5.h"
#include "idFileTypes/WAD7.h"

#include "Oodle.h"
#include "Utilities.h"

namespace fs = std::filesystem;

namespace HAYDEN
{
    // User-friendly struct for managing .resources data
    struct ResourceEntry
    {
        uint64_t DataOffset = 0;
        uint64_t DataSize = 0;
        uint64_t DataSizeUncompressed = 0;
        uint64_t StreamResourceHash = 0;
        uint32_t Version = 0;
        uint16_t CompressionMode = 0;
        std::string Name;
        std::string Type;
        bool isIDCL = 0;
    };

    enum ArchiveType
    {
        TYPE_RESOURCES = 0,
        TYPE_PK5 = 1,
        TYPE_WAD7 = 2,
        TYPE_UNSUPPORTED = 999
    };

    class ResourceFileReader
    {
        public:
            fs::path ResourceFilePath;

            std::vector<ResourceEntry> ParseResourceFile();
            std::vector<ResourceEntry> ParsePK5();
            std::vector<ResourceEntry> ParseWAD7();
            uint64_t CalculateStreamDBIndex(uint64_t resourceId, const int mipCount = -6) const;
            std::vector<uint8_t> GetEmbeddedFileHeader(const std::string resourcePath, const uint64_t fileOffset, const uint64_t compressedSize, const uint64_t decompressedSize);
            ResourceFileReader(const fs::path resourceFilePath) { ResourceFilePath = resourceFilePath; }
    };
}