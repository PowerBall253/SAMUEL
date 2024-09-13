#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "idFileTypes/ResourceFile.h"

#include "ResourceFileReader.h"
#include "Utilities.h"

namespace fs = std::filesystem;

namespace HAYDEN
{
    class RAWExportTask
    {
        public:

            bool Export(const fs::path exportPath, const std::string resourcePath);
            RAWExportTask(const ResourceEntry resourceEntry);

        private:

            std::string _FileName;        
            uint64_t _ResourceDataOffset = 0;
            uint64_t _ResourceDataLength = 0;
            uint64_t _ResourceDataLengthDecompressed = 0;
    };
}