#include "PNG.h"
#include "../../../vendor/DirectXTex/DirectXTex/DirectXTex.h"
#include "source/core/Utilities.h"

#ifdef __linux__
#include "../../../vendor/DirectXTex/Auxiliary/DirectXTexPNG.h"
#endif

namespace HAYDEN
{
    // Convert DDS file to PNG using DirectXTex
    bool PNGFile::ConvertDDStoPNG(std::vector<uint8_t> inputDDS, ImageType imageType, fs::path exportPath, bool reconstructZ)
    {
#ifdef _WIN32
        HRESULT initCOM = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(initCOM))
        {
            fprintf(stderr, "ERROR: Failed to initialize the COM library. \n");
            return false;
        }
#endif

        // Read DDS binary data into DirectX::ScratchImage
        DirectX::ScratchImage scratchImage;
        if (DirectX::LoadFromDDSMemory(inputDDS.data(), inputDDS.size(), DirectX::DDS_FLAGS_NONE, nullptr, scratchImage) != 0)
        {
            fprintf(stderr, "ERROR: Failed to read DDS data from given file. \n");
            return false;
        }

        // Decompress DDS image and storeRead DDS binary data into DirectX::ScratchImage
        DirectX::ScratchImage scratchImageDecompressed;
        if (DirectX::Decompress(*scratchImage.GetImage(0, 0, 0), DXGI_FORMAT_UNKNOWN, scratchImageDecompressed) != 0)
        {
            // If decompression failed, just use the data as-is.
            // Most images in DOOM Eternal have block compression, but some do not.
            scratchImageDecompressed = std::move(scratchImage);
        }

        // Construct a temporary image object to check the format
        auto tmpImage = scratchImageDecompressed.GetImage(0, 0, 0);

        // BC5 normals need to be converted to an intermediate format before converting to PNG
        if (tmpImage->format == DXGI_FORMAT_R8G8_UNORM)
        {
            // Convert to RGBA8_UNORM
            DirectX::ScratchImage rgba8Image;
            DirectX::Convert(*scratchImageDecompressed.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgba8Image);
            scratchImageDecompressed = std::move(rgba8Image);

            // Reconstruct the blue channel if requested
            if (reconstructZ)
            {
                HRESULT restoreZ;
                DirectX::ScratchImage scratchReconstructedZ;
                DirectX::TexMetadata imgMeta = scratchImageDecompressed.GetMetadata();

                restoreZ = DirectX::TransformImage(scratchImageDecompressed.GetImages(), scratchImageDecompressed.GetImageCount(), scratchImageDecompressed.GetMetadata(),
                    [&](DirectX::XMVECTOR* outPixels, const DirectX::XMVECTOR* inPixels, size_t w, size_t y)
                    {
                        static const DirectX::XMVECTORU32 s_selectz = { { {DirectX::XM_SELECT_0, DirectX::XM_SELECT_0, DirectX::XM_SELECT_1, DirectX::XM_SELECT_0 } } };
                        UNREFERENCED_PARAMETER(y);

                        for (size_t j = 0; j < w; ++j)
                        {
                            const DirectX::XMVECTOR value = inPixels[j];
                            DirectX::XMVECTOR z;

                            DirectX::XMVECTOR x2 = DirectX::XMVectorMultiplyAdd(value, DirectX::g_XMTwo, DirectX::g_XMNegativeOne);
                            x2 = DirectX::XMVectorSqrt(DirectX::XMVectorSubtract(DirectX::g_XMOne, DirectX::XMVector2Dot(x2, x2)));
                            z = DirectX::XMVectorMultiplyAdd(x2, DirectX::g_XMOneHalf, DirectX::g_XMOneHalf);

                            outPixels[j] = XMVectorSelect(value, z, s_selectz);
                        }
                    }, scratchReconstructedZ);

                if (FAILED(restoreZ))
                    return false;

                scratchImageDecompressed = std::move(scratchReconstructedZ);
            }
        }

        // Construct final raw image for converting to PNG
        auto rawImage = scratchImageDecompressed.GetImage(0, 0, 0);

        // Create out directory
        fs::path exportDir = exportPath;
        exportDir.remove_filename();
        if (!mkpath(exportDir)) {
            fprintf(stderr, "Error: Failed to create directories for file: %s \n", exportPath.string().c_str());
            return false;
        }

        // Convert to PNG and save
#ifdef _WIN32
        // "\\?\" alongside the wide string functions is used to bypass PATH_MAX
        // Check https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd for details
        std::wstring wPath = L"\\\\?\\" + exportPath.wstring();

        if (DirectX::SaveToWICFile(*rawImage, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), wPath.c_str(), &GUID_WICPixelFormat32bppBGRA) != 0) {
            fprintf(stderr, "ERROR: Failed to write new PNG file. \n");
            return false;
        }
#else
        if (DirectX::SaveToPNGFile(*rawImage, exportPath.wstring().c_str()) != 0) {
            fprintf(stderr, "ERROR: Failed to write new PNG file. \n");
            return false;
        }
#endif

        return true;
    }
}