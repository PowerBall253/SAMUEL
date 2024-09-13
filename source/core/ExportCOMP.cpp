#include "ExportCOMP.h"

namespace HAYDEN
{
    COMPExportTask::COMPExportTask(const ResourceEntry resourceEntry)
    {
        _FileName = resourceEntry.Name;
        _ResourceDataOffset = resourceEntry.DataOffset;
        _ResourceDataLength = resourceEntry.DataSize;
        _ResourceDataLengthDecompressed = resourceEntry.DataSizeUncompressed;
        return;
    }

    bool COMPExportTask::Export(fs::path exportPath, const std::string resourcePath, bool isPK5)
    {
        ResourceFileReader resourceFile(resourcePath);
        std::vector<uint8_t> compFile = resourceFile.GetEmbeddedFileHeader(resourcePath, _ResourceDataOffset, _ResourceDataLength, _ResourceDataLengthDecompressed);

        if (compFile.empty())
            return 0;

        // Read compressed and decompressed size from comp file header
        uint32_t decompressedSize = *(uint32_t*)(compFile.data() + 0);
        uint32_t compressedSize = *(uint32_t*)(compFile.data() + 8);

        // Remove header and decompress remaining data
        compFile.erase(compFile.begin(), compFile.begin() + 16);

        // If compressed size = -1, file isn't compressed. Seen in some wad7s
        if (compressedSize != -1)
            compFile = oodleDecompress(compFile, decompressedSize);

        if (compFile.empty())
        {
            fprintf(stderr, "Error: Failed to decompress: %s \n", _FileName.c_str());
            return 0;
        }

        if (isPK5)
        {
            // Delete temporary IDCL file we created, write extracted file here
            fs::remove(resourcePath);
            exportPath = fs::path(resourcePath);
        }

        return writeToFilesystem(compFile, exportPath);
    }
}