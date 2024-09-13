#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "idFileTypes/ResourceFile.h"

#include "Oodle.h"
#include "ResourceFileReader.h"
#include "Utilities.h"

namespace fs = std::filesystem;

namespace HAYDEN
{
    class COMPExportTask
    {
        public:

            bool Export(fs::path exportPath, const std::string resourcePath, bool isPK5 = false);
            COMPExportTask(const ResourceEntry resourceEntry);

        private:

            std::string _FileName;
            uint64_t _ResourceDataOffset = 0;
            uint64_t _ResourceDataLength = 0;
            uint64_t _ResourceDataLengthDecompressed = 0;
    };
}