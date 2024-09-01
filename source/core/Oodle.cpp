#include "Oodle.h"

namespace HAYDEN
{
    // Decompress using Oodle DLL
    std::vector<uint8_t> oodleDecompress(std::vector<uint8_t> compressedData, const uint64_t decompressedSize)
    {
        std::vector<uint8_t> output(decompressedSize + SAFE_SPACE);
        uint64_t outbytes = 0;

        // Decompress using ooz library
        outbytes = Kraken_Decompress(compressedData.data(), compressedData.size(), output.data(), decompressedSize);

        if (outbytes == 0)
        {
            fprintf(stderr, "Error: failed to decompress with Ooz library.\n\n");
            return std::vector<uint8_t>();
        }

        return std::vector<uint8_t>(output.begin(), output.begin() + outbytes);
    }
}