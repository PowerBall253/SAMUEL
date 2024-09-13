#include "ExportManager.h"

namespace HAYDEN
{
    ExportManager::ExportManager(ArchiveType archiveType)
    {
        _ArchiveType = archiveType;
        if (_ArchiveType == ArchiveType::TYPE_PK5)
            _IsPK5 = 1;

        if (_ArchiveType == ArchiveType::TYPE_WAD7)
            _IsPK5 = 1;
    }

    // Returns the name of the resource folder (e.g. "gameresources_patch1") where files will be exported to
    std::string ExportManager::GetResourceFolder(const std::string resourcePath)
    {
        std::string resourceFolder;
        size_t offset1 = resourcePath.rfind("/");
        size_t offset2 = resourcePath.find(".");

        // this shouldn't happen, but if it somehow does, we just won't use the resourceFolder in our export path.
        if (offset1 < 8 || offset2 < offset1)
            return resourceFolder;

        // get only the resource folder name, without path or file extension
        size_t length = offset2 - offset1;
        resourceFolder = resourcePath.substr(offset1 + 1, length - 1);

        // separates dlc hub from regular hub folder
        if (resourcePath.substr(offset1 - 8, 8) == "/dlc/hub")
            resourceFolder = "dlc_" + resourceFolder;

        return resourceFolder;
    }

    // Return the full path where a file will be exported to
    fs::path ExportManager::BuildOutputPath(std::string filePath, fs::path outputDirectory, const ExportType exportType, const std::string resourceFolder)
    {
        switch (exportType)
        {
            case ExportType::COMP:
            case ExportType::DECL:
            case ExportType::IDCL:
                outputDirectory = outputDirectory / resourceFolder;
                break;
            case ExportType::BIM:
                filePath = filePath + ".png";
                outputDirectory = outputDirectory / resourceFolder;
                break;
            case ExportType::LWO:
            case ExportType::MD6:
                if (_ArchiveType == ArchiveType::TYPE_RESOURCES)
                {
                    filePath = fs::path(filePath).filename().replace_extension("").string();
                    outputDirectory.replace_filename("modelExports");
                }
                else
                {
                    outputDirectory = outputDirectory / resourceFolder;
                }
                break;
        }
        outputDirectory = outputDirectory.append(filePath);
        outputDirectory.make_preferred();
        return outputDirectory;
    }

    // Main file export function
    bool ExportManager::ExportFiles(GLOBAL_RESOURCES* globalResources, std::vector<ResourceEntry>& resourceData, const std::string resourcePath, const std::vector<StreamDBFile>& streamDBFiles, const fs::path outputDirectory, const std::vector<std::vector<std::string>> filesToExport)
    {
        // Abort if this function was called without any files selected for extraction.
        if (filesToExport.size() == 0)
            return 0;

        // Determine output directory
        std::string resourceFolder = GetResourceFolder(resourcePath);

        // Create seperate export lists for each type of file
        // Some files have the same names but different versions (types), so we need to keep this separate
        for (int i = 0; i < filesToExport.size(); i++)
        {
            int fileType = std::stoi(filesToExport[i][2]);

            // TODO: Changing this here is not intuitive and should be reworked
            // We should segment ACTUAL version from DISPLAYED version to user before it gets passed to Qt
            if (filesToExport[i][3] == "1")
                fileType = 99;

            switch (fileType)
            {
                case 0:
                    _DECLFileNames.push_back(filesToExport[i][0]);
                    break;
                case 1:
                    _COMPFileNames.push_back(filesToExport[i][0]);
                    break;
                case 21:
                    _BIMFileNames.push_back(filesToExport[i][0]);
                    break;
                case 31:
                    _MD6FileNames.push_back(filesToExport[i][0]);
                    break;
                case 67:
                    _LWOFileNames.push_back(filesToExport[i][0]);
                    break;
                case 99:
                    _IDCLFileNames.push_back(filesToExport[i][0]);
                    break;
                default:
                    _RAWFileNames.push_back(filesToExport[i][0]);
                    break;
            }
        }

        // Iterate through currently loaded .resources data and construct _ExportJobQueue
        for (uint64_t i = 0; i < resourceData.size(); i++)
        {
            ExportTask task;
            ResourceEntry thisEntry = resourceData[i];

            // Skip this entry if it contains no data to extract. 
            // This can happen with certain files that have been removed from the game.
            if (thisEntry.DataSize == 0)
                continue;

            if (thisEntry.isIDCL)
                thisEntry.Version = 99;

            // Check the .resources entries against our list of files to extract.
            switch (thisEntry.Version)
            {
                case 0:
                    task.Type = ExportType::DECL;
                    if (std::find(_DECLFileNames.begin(), _DECLFileNames.end(), thisEntry.Name) == _DECLFileNames.end())
                        continue;
                    break;
                case 1:
                    task.Type = ExportType::COMP;
                    if (std::find(_COMPFileNames.begin(), _COMPFileNames.end(), thisEntry.Name) == _COMPFileNames.end())
                        continue;
                    break;
                case 21:
                    task.Type = ExportType::BIM;
                    if (std::find(_BIMFileNames.begin(), _BIMFileNames.end(), thisEntry.Name) == _BIMFileNames.end())
                        continue;
                    break;
                case 31:
                    task.Type = ExportType::MD6;
                    if (std::find(_MD6FileNames.begin(), _MD6FileNames.end(), thisEntry.Name) == _MD6FileNames.end())
                        continue;
                    break;
                case 67:
                    task.Type = ExportType::LWO;
                    if (std::find(_LWOFileNames.begin(), _LWOFileNames.end(), thisEntry.Name) == _LWOFileNames.end())
                        continue;
                    break;
                case 99:
                    task.Type = ExportType::IDCL;
                    if (std::find(_IDCLFileNames.begin(), _IDCLFileNames.end(), thisEntry.Name) == _IDCLFileNames.end())
                        continue;
                    break;
                default:
                    task.Type = ExportType::RAW;
                    if (std::find(_RAWFileNames.begin(), _RAWFileNames.end(), thisEntry.Name) == _RAWFileNames.end())
                        continue;
                    break;
            }

            // If a match is found, add this entry to _ExportJobQueue
            task.Entry = thisEntry;
            task.ExportPath = BuildOutputPath(thisEntry.Name, outputDirectory, task.Type, resourceFolder);
            _ExportJobQueue.push_back(task);
        }

        // Iterate through _ExportJobQueue and complete the file export
        for (int i = 0; i < _ExportJobQueue.size(); i++)
        {
            switch (_ExportJobQueue[i].Type)
            {
                case ExportType::BIM:
                {
                    BIMExportTask bimExportTask(_ExportJobQueue[i].Entry);
                    _ExportJobQueue[i].Result = bimExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath, streamDBFiles, true, _IsPK5);
                    break;
                }
                case ExportType::COMP:
                {
                    COMPExportTask compExportTask(_ExportJobQueue[i].Entry);
                    _ExportJobQueue[i].Result = compExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath, _IsPK5);
                    break;
                }
                case ExportType::DECL:
                {
                    DECLExportTask declExportTask(_ExportJobQueue[i].Entry);
                    _ExportJobQueue[i].Result = declExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath);
                    break;
                }
                case ExportType::MD6:
                {
                    ModelExportTask modelExportTask(_ExportJobQueue[i].Entry);
                    modelExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath, streamDBFiles, resourceData, globalResources, 31);
                    break;
                }
                case ExportType::LWO:
                {
                    ModelExportTask modelExportTask(_ExportJobQueue[i].Entry);
                    modelExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath, streamDBFiles, resourceData, globalResources, 67, _IsPK5);
                    break;
                }
                case ExportType::IDCL:
                {
                    IDCLExportTask idclExportTask(_ExportJobQueue[i].Entry);
                    _ExportJobQueue[i].Result = idclExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath);

                    // If successful, open the exported IDCL container and extract embedded files
                    if (_ExportJobQueue[i].Result == 1)
                    {
                        /* Note:
                        ** All known IDCLs exported this way contain only TWO file entries (actually one file, split into two parts)
                        ** The "version" assigned to the first entry determines how the export is handled. 21 = BIM, 67 = LWO etc.
                        ** The second entry is always version 0, type rs_emb_sfile
                        ** This second part is similar to what we'd extract from .streamdb when working with standard .resources files. */

                        // Read IDCL container for list of embedded files
                        ResourceFileReader reader(_ExportJobQueue[i].ExportPath);
                        std::vector<ResourceEntry> embeddedResourceData = reader.ParseResourceFile();

                        // Determine what kind of file is contained within
                        std::string resourceName = embeddedResourceData[0].Name;
                        std::string resourceType = embeddedResourceData[0].Type;
                        std::string resourceVersion = std::to_string(embeddedResourceData[0].Version);

                        if (resourceVersion == "1" && resourceType == "compfile")
                        {
                            std::vector<uint8_t> embeddedFile = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[0].DataOffset, embeddedResourceData[0].DataSize, embeddedResourceData[0].DataSizeUncompressed);
                            if (embeddedFile.empty())
                                return 0;

                            writeToFilesystem(embeddedFile, _ExportJobQueue[i].ExportPath);

                            // Update offsets for export manager
                            embeddedResourceData[0].DataOffset = 0;
                            embeddedResourceData[0].DataSize = embeddedResourceData[0].DataSizeUncompressed;

                            // Initialize a new export manager
                            ExportManager embeddedExportManager(_ArchiveType);

                            // Build export list for export manager
                            std::vector<std::vector<std::string>> exportList;
                            std::vector<std::string> rowText = { resourceName, resourceType, resourceVersion, "0" };
                            exportList.push_back(rowText);

                            // Open new export manager tasks
                            embeddedExportManager.ExportFiles(globalResources, embeddedResourceData, _ExportJobQueue[i].ExportPath.string(), streamDBFiles, outputDirectory, exportList);
                        }
                        else if (resourceVersion == "21")
                        {
                            // Read header (sometimes this is the entire image, in case of WAD7)
                            std::vector<uint8_t> bimHeader = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[0].DataOffset, embeddedResourceData[0].DataSize, embeddedResourceData[0].DataSizeUncompressed);
                            if (bimHeader.empty())
                                return 0;

                            // Check if there are additional files in the IDCL
                            // If so, this should be the image body, combine them
                            if (embeddedResourceData.size() > 1)
                            {
                                std::vector<uint8_t> bimBody = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[1].DataOffset, embeddedResourceData[1].DataSize, embeddedResourceData[1].DataSizeUncompressed);
                                bimHeader.insert(bimHeader.end(), bimBody.begin(), bimBody.end());
                            }

                            writeToFilesystem(bimHeader, _ExportJobQueue[i].ExportPath);

                            // Update offsets for export manager
                            embeddedResourceData[0].DataOffset = 0;
                            embeddedResourceData[0].DataSize = embeddedResourceData[0].DataSizeUncompressed;

                            // Initialize a new export manager
                            ExportManager embeddedExportManager(_ArchiveType);

                            // Build export list for export manager
                            std::vector<std::vector<std::string>> exportList;
                            std::vector<std::string> rowText = { resourceName, resourceType, resourceVersion, "0"};
                            exportList.push_back(rowText);

                            // Open new export manager tasks
                            embeddedExportManager.ExportFiles(globalResources, embeddedResourceData, _ExportJobQueue[i].ExportPath.string(), streamDBFiles, outputDirectory, exportList);
                        }
                        else if (resourceVersion == "67")
                        {
                            // Read header (sometimes this is the entire model, in case of WAD7)
                            std::vector<uint8_t> lwoHeader = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[0].DataOffset, embeddedResourceData[0].DataSize, embeddedResourceData[0].DataSizeUncompressed);
                            if (lwoHeader.empty())
                                return 0; 

                            // Check if there are additional files in the IDCL
                            // If so, this should be the model geometry, combine them
                            if (embeddedResourceData.size() > 1)
                            {
                                std::vector<uint8_t> lwoBody = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[1].DataOffset, embeddedResourceData[1].DataSize, embeddedResourceData[1].DataSizeUncompressed);
                                lwoHeader.insert(lwoHeader.end(), lwoBody.begin(), lwoBody.end());
                            }
                            
                            writeToFilesystem(lwoHeader, _ExportJobQueue[i].ExportPath);

                            // Update offsets for export manager
                            embeddedResourceData[0].DataOffset = 0;
                            embeddedResourceData[0].DataSize = embeddedResourceData[0].DataSizeUncompressed;

                            // Initialize a new export manager
                            ExportManager embeddedExportManager(_ArchiveType);

                            // Build export list for export manager
                            std::vector<std::vector<std::string>> exportList;
                            std::vector<std::string> rowText = { resourceName, resourceType, resourceVersion, "0" };
                            exportList.push_back(rowText);

                            // Open new export manager tasks
                            embeddedExportManager.ExportFiles(globalResources, embeddedResourceData, _ExportJobQueue[i].ExportPath.string(), streamDBFiles, outputDirectory, exportList);
                        }
                        else if (embeddedResourceData.size() == 1)
                        {
                            std::vector<uint8_t> embeddedFile = reader.GetEmbeddedFileHeader(_ExportJobQueue[i].ExportPath.string(), embeddedResourceData[0].DataOffset, embeddedResourceData[0].DataSize, embeddedResourceData[0].DataSizeUncompressed);
                            if (embeddedFile.empty())
                                return 0;

                            writeToFilesystem(embeddedFile, _ExportJobQueue[i].ExportPath);
                        }
                    }
                    break;
                }
                case ExportType::RAW:
                {
                    RAWExportTask rawExportTask(_ExportJobQueue[i].Entry);
                    _ExportJobQueue[i].Result = rawExportTask.Export(_ExportJobQueue[i].ExportPath, resourcePath);
                    break;
                }
                default:
                    break;
            }
        }

        return 1;
    }
}