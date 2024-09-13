#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "Common.h"
#include "ExportBIM.h"
#include "ExportCOMP.h"
#include "ExportDECL.h"
#include "ExportIDCL.h"
#include "ExportModel.h"
#include "ExportRAW.h"
#include "ResourceFileReader.h"

namespace fs = std::filesystem;

namespace HAYDEN
{
    enum class ExportType
    {
        DECL = 0,
        COMP = 1,
        BIM = 21,
        MD6 = 31,
        LWO = 67,
        IDCL = 99,
        RAW = 999
    };

    class ExportTask
    {
        public:
            bool Result = 0;
            fs::path ExportPath;
            ExportType Type = ExportType::DECL;
            ResourceEntry Entry;
    };

    class ExportManager
    {
        public:
            bool HasError = 0;
            std::string LastErrorMessage;
            std::string LastErrorDetail;
            std::string GetResourceFolder(const std::string resourcePath);
            fs::path BuildOutputPath(std::string filePath, fs::path outputDirectory, const ExportType exportType, const std::string resourceFolder);
            bool ExportFiles(GLOBAL_RESOURCES* globalResources, std::vector<ResourceEntry>& resourceData, const std::string resourcePath, const std::vector<StreamDBFile>& streamDBFiles, const fs::path outputDirectory, const std::vector<std::vector<std::string>> filesToExport);
            ExportManager(ArchiveType archiveType);

        private:
            bool _IsPK5 = 0;
            ArchiveType _ArchiveType;
            std::vector<ExportTask> _ExportJobQueue;     
            std::vector<std::string> _BIMFileNames;
            std::vector<std::string> _LWOFileNames;
            std::vector<std::string> _MD6FileNames;
            std::vector<std::string> _DECLFileNames;
            std::vector<std::string> _COMPFileNames;
            std::vector<std::string> _IDCLFileNames;
            std::vector<std::string> _RAWFileNames;
    };
}