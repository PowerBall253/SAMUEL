#include "ExportBIM.h"

namespace HAYDEN
{
    // Constructor
    BIMExportTask::BIMExportTask(const ResourceEntry resourceEntry)
    {
        _FileName = resourceEntry.Name;
        _ResourceDataOffset = resourceEntry.DataOffset;
        _ResourceDataLength = resourceEntry.DataSize;
        _ResourceDataLengthDecompressed = resourceEntry.DataSizeUncompressed;
        _ResourceID = resourceEntry.StreamResourceHash;
        return;
    }

    // Locate and extract embedded image data from .streamdb file
    bool BIMExportTask::LocateFileInStreamDB(const std::vector<StreamDBFile>& streamDBFiles)
    {
        for (int i = 0; i < streamDBFiles.size(); i++)
        {
            _StreamDBEntry = streamDBFiles[i].LocateStreamDBEntry(_StreamedDataHash, _StreamedDataLength);
            if (_StreamDBEntry.Offset16 > 0)
            {
                _StreamDBNumber = i;
                _StreamDBFilePath = streamDBFiles[i].FilePath; 
                return 1;
            }
        }
        return 0;
    }

    // Main export function for BIM files.
    // Convert BIM file to PNG format and write to local filesystem. 
    // Return 1 for success, 0 for failure.
    bool BIMExportTask::Export(fs::path exportPath, const std::string resourcePath, const std::vector<StreamDBFile>& streamDBFiles, bool reconstructZ, bool isPK5)
    {
        std::vector<uint8_t> rawImageData;
        ResourceFileReader resourceFile(resourcePath);

        // Extract BIM header from .resources file and read it
        std::vector<uint8_t> binaryData = resourceFile.GetEmbeddedFileHeader(resourcePath, _ResourceDataOffset, _ResourceDataLength, _ResourceDataLengthDecompressed);

        if (binaryData.empty())
            return 0; 

        // Serialize binary data from our extracted BIM header
        _BIM.Serialize(binaryData);
        _ImgType = _BIM.Header.TextureFormat; 
        _ImgPixelWidth = _BIM.Header.PixelWidth;
        _ImgPixelHeight = _BIM.Header.PixelHeight;
        _ImgMipCount = _BIM.Header.StreamDBMipCount;

        // For images in .PK5 archives
        if (isPK5)
        {
            // No streamdb here, find the largest mip
            // Sometimes BIM header contains the whole file, check if any data is here (found during _BIM.Serialize())
            if (_BIM.RawImageData.empty())
            {
                // We only care about the largest mip, starts immediately after the header
                rawImageData.resize(_BIM.MipMaps[0].DecompressedSize);
                rawImageData = resourceFile.GetEmbeddedFileHeader(resourcePath, _ResourceDataLength, _BIM.MipMaps[0].CompressedSize, _BIM.MipMaps[0].DecompressedSize);
                _StreamedDataLengthDecompressed = _BIM.MipMaps[0].DecompressedSize;
            }
            else
            {
                // All data is in BIM header, no 2nd file to extract
                rawImageData = GetBIMRawImage();
            }

            // Delete temporary IDCL file we created, write extracted image here
            fs::remove(resourcePath);
            fs::path tempPath = fs::path(resourcePath).replace_extension(".png");
            exportPath = fs::path(tempPath);
        }

        // For images in .resources files
        if (!isPK5)
        {
            // Get StreamDB Index and Size from serialized BIM data
            _StreamedDataLengthDecompressed = _BIM.MipMaps[0].DecompressedSize;
            _StreamedDataLength = _BIM.MipMaps[0].CompressedSize;

            // Sometimes a file is in streamdb even if BoolIsStreamed == 0, so we have to check if the largest mip is compressed.
            if (_BIM.Header.BoolIsStreamed == 0 && _BIM.MipMaps[0].BoolIsCompressed == 0)
                _IsStreamed = 0;

            // Convert resourceID to streamFileID
            _StreamedDataHash = resourceFile.CalculateStreamDBIndex(_ResourceID, _ImgMipCount);

            // Locate and extract the BIM image from .streamdb file
            if (_IsStreamed)
            {
                // If no match is found, reduce _StreamedDataHash by 1 and search again. 
                // This is necessary for locating some BIM files (reason for this is unknown)
                if (!LocateFileInStreamDB(streamDBFiles))
                {
                    _StreamedDataHash--;
                    if (!LocateFileInStreamDB(streamDBFiles))
                        return 0; // abort
                }

                // Extract streaming image data from .streamdb file.
                rawImageData = streamDBFiles[_StreamDBNumber].GetEmbeddedFile(_StreamDBFilePath, _StreamDBEntry);

                // Decompress the streamed image data if needed (almost always).
                if (_StreamDBEntry.CompressedSize != _StreamedDataLengthDecompressed)
                {
                    rawImageData = oodleDecompress(rawImageData, _StreamedDataLengthDecompressed);
                    if (rawImageData.empty())
                    {
                        fprintf(stderr, "Error: Failed to decompress: %s \n", _FileName.c_str());
                        return 0;
                    }
                }
            }
            else
            {
                // Non-streamed image, can grab image data directly from extracted .resources entry
                rawImageData = GetBIMRawImage();
            }
        }

        // Construct a DDS file header from serialized BIM data
        DDSHeaderBuilder ddsBuilder(_ImgPixelWidth, _ImgPixelHeight, _StreamedDataLengthDecompressed, static_cast<ImageType>(_ImgType));
        std::vector<uint8_t> ddsFile = ddsBuilder.ConvertToByteVector();

        // Merge header and data into one byte vector
        ddsFile.insert(ddsFile.end(), rawImageData.begin(), rawImageData.end());

        // Convert DDS file to PNG format
        PNGFile pngFile;
        return pngFile.ConvertDDStoPNG(ddsFile, static_cast<ImageType>(_ImgType), exportPath, reconstructZ);
    }
}