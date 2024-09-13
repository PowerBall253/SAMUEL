#include "WAD7.h"

namespace HAYDEN
{
    // Reads binary .WAD7 file from local filesystem
    WAD7::WAD7(const fs::path filePath)
    {
        FilePath = filePath.string();
        FILE* f = fopen(FilePath.c_str(), "rb");
        if (f == NULL)
        {
            fprintf(stderr, "ERROR : WAD7 : Failed to open %s for reading.\n", FilePath.c_str());
            return;
        }

        if (f != NULL)
        {
            // Read .WAD7 file header
            fseek(f, 0, SEEK_SET);
            fread(&Header, sizeof(WAD7_HEADER), 1, f);

            endianSwap64(Header.IndexStart);
            endianSwap64(Header.IndexSize);

            // Read entry count
            fseek64(f, Header.IndexStart, SEEK_SET);
            fread(&EntryCount, sizeof(uint32_t), 1, f);
            endianSwap32(EntryCount);

            FileEntries.resize(EntryCount);
            EntryNames.resize(EntryCount);
            EntryTypes.resize(EntryCount);
            EntryVersions.resize(EntryCount);
            EmbeddedTypes.resize(EntryCount);

            // Read .WAD7 file entries
            for (uint64_t i = 0; i < FileEntries.size(); i++)
            {
                uint32_t stringLength = 0;
                fread(&stringLength, sizeof(uint32_t), 1, f);

                char* stringBuffer = new char[stringLength];
                stringBuffer[stringLength] = '\0';
                fread(stringBuffer, stringLength, 1, f);
                EntryNames[i] = stringBuffer;

                fread(&FileEntries[i], sizeof(WAD7_ENTRY), 1, f);

                endianSwap64(FileEntries[i].DataStartOffset);
                endianSwap32(FileEntries[i].UncompressedSize);
                endianSwap32(FileEntries[i].CompressedSize);
                endianSwap32(FileEntries[i].CompressionMode);
            }

            // Read first 4 bytes of each entry to identify entry type (IDCL or plaintext)
            for (uint64_t i = 0; i < FileEntries.size(); i++)
            {
                uint32_t buffer;
                fseek64(f, FileEntries[i].DataStartOffset, SEEK_SET);
                fread(&buffer, 4, 1, f);

                // For IDCL files, we want to identify the contents to display in GUI
                if (buffer == 1279476809)
                {
                    EntryTypes[i] = EntryType::TYPE_IDCL;
                    
                    // Read IDCL header
                    fseek64(f, FileEntries[i].DataStartOffset, SEEK_SET);
                    ResourceFileHeader idclHeader;
                    fread(&idclHeader, sizeof(ResourceFileHeader), 1, f);            
                    
                    // Some WAD7 entries are empty, reason unknown
                    if (idclHeader.NumFileEntries != 0)
                    {
                        // Get version - for this type of IDCL only the first entry matters
                        fseek64(f, FileEntries[i].DataStartOffset, SEEK_SET);
                        fseek64(f, idclHeader.AddrEntries, SEEK_CUR);
                        ResourceFileEntry idclEntry;
                        fread(&idclEntry, sizeof(ResourceFileEntry), 1, f);
                        EntryVersions[i] = idclEntry.Version;

                        // Get type string
                        uint64_t numStrings;
                        uint64_t addrStringCount = idclHeader.AddrEntries + (sizeof(ResourceFileEntry) * idclHeader.NumFileEntries);
                        fseek64(f, FileEntries[i].DataStartOffset, SEEK_SET);
                        fseek64(f, addrStringCount, SEEK_CUR);
                        fread(&numStrings, sizeof(uint64_t), 1, f);

                        std::vector<uint64_t> stringOffsets;
                        stringOffsets.resize(numStrings + 1);
                        for (uint64_t j = 0; j < numStrings; j++)
                            fread(&stringOffsets[j], 8, 1, f);

                        stringOffsets[numStrings] = idclHeader.AddrDependencyEntries;

                        // Read strings into vector
                        std::vector<std::string> stringEntries;
                        stringEntries.resize(numStrings);
                        for (uint64_t i = 0; i < numStrings; i++)
                        {
                            int stringLength = stringOffsets[i + 1] - stringOffsets[i];
                            char* stringBuffer = new char[stringLength];
                            fread(stringBuffer, stringLength, 1, f);
                            stringEntries[i] = stringBuffer;
                        }
                        EmbeddedTypes[i] = stringEntries[idclEntry.PathTuple_OffsetType];
                    }
                    else
                    {
                        EntryVersions[i] = 9999;
                        EmbeddedTypes[i] = "Empty File - DO NOT EXPORT";
                    }

                }
                else
                {
                    EntryTypes[i] = EntryType::TYPE_PLAINTEXT;
                    EntryVersions[i] = 0;
                }
            }
        }
        fclose(f);
    }
}