#include "ExportIDCL.h"

namespace HAYDEN
{
    IDCLExportTask::IDCLExportTask(const ResourceEntry resourceEntry)
    {
        _FileName = resourceEntry.Name;
        _ResourceDataOffset = resourceEntry.DataOffset;
        _ResourceDataLength = resourceEntry.DataSize;
        _ResourceDataLengthDecompressed = resourceEntry.DataSizeUncompressed;
        return;
    }

    bool IDCLExportTask::Export(const fs::path exportPath, const std::string resourcePath)
    {
        ResourceFileReader resourceFileReader(resourcePath);
        std::vector<uint8_t> fileData = resourceFileReader.GetEmbeddedFileHeader(resourcePath, _ResourceDataOffset, _ResourceDataLength, _ResourceDataLengthDecompressed);

        if (fileData.empty())
            return 0;

        return writeToFilesystem(fileData, exportPath);
    }
}