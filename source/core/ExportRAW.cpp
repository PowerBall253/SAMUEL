#include "ExportRAW.h"

namespace HAYDEN
{
    /* This is literally a copy/paste of DECL file handling and may be removed later */
    /* For now I want to keep it on a separate logic path in case any special handling is required */
    RAWExportTask::RAWExportTask(const ResourceEntry resourceEntry)
    {
        _FileName = resourceEntry.Name;
        _ResourceDataOffset = resourceEntry.DataOffset;
        _ResourceDataLength = resourceEntry.DataSize;
        _ResourceDataLengthDecompressed = resourceEntry.DataSizeUncompressed;
        return;
    }

    bool RAWExportTask::Export(const fs::path exportPath, const std::string resourcePath)
    {
        ResourceFileReader resourceFileReader(resourcePath);
        std::vector<uint8_t> fileData = resourceFileReader.GetEmbeddedFileHeader(resourcePath, _ResourceDataOffset, _ResourceDataLength, _ResourceDataLengthDecompressed);

        if (fileData.empty())
            return 0;

        return writeToFilesystem(fileData, exportPath);
    }
}