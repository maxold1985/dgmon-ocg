#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace hc {
namespace xor_archive {

// XOR is reversible obfuscation, NOT secure encryption.
// Key is repeated for the full binary stream; an empty key is rejected.
bool transformFile(const std::string& inputPath,
                   const std::string& outputPath,
                   const std::string& key,
                   std::string* error = 0);

// DAT envelope: 8-byte signature + 8-byte little-endian ZIP length + XOR payload.
// No external ZIP library required. decodeDat validates the recovered ZIP signature.
bool encodeZipToDat(const std::string& zipPath,
                    const std::string& datPath,
                    const std::string& key,
                    std::string* error = 0);
bool decodeDatToZip(const std::string& datPath,
                    const std::string& zipPath,
                    const std::string& key,
                    std::string* error = 0);

} // namespace xor_archive
} // namespace hc
