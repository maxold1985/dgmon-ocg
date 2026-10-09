#include "xor_archive.h"
#include <cstdio>
#include <fstream>
#include <limits>
#include <vector>

namespace hc {
namespace xor_archive {
namespace {
const unsigned char kMagic[8] = {'H','C','X','O','R','0','1','\n'};
const std::size_t kBufferSize = 64 * 1024;

bool fail(std::string* error, const std::string& reason) {
    if (error) *error = reason;
    return false;
}

bool sameFile(const std::string& a, const std::string& b) {
    // Reject identical paths, including common Windows case differences.
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        unsigned char x = static_cast<unsigned char>(a[i]);
        unsigned char y = static_cast<unsigned char>(b[i]);
        if (x >= 'A' && x <= 'Z') x = static_cast<unsigned char>(x + ('a' - 'A'));
        if (y >= 'A' && y <= 'Z') y = static_cast<unsigned char>(y + ('a' - 'A'));
        if (x != y) return false;
    }
    return true;
}

bool hasZipSignature(const unsigned char* p) {
    // Normal ZIP, empty ZIP, or spanned ZIP.
    return p[0] == 'P' && p[1] == 'K' &&
           ((p[2] == 3 && p[3] == 4) ||
            (p[2] == 5 && p[3] == 6) ||
            (p[2] == 7 && p[3] == 8));
}

bool readLength(std::ifstream& file, std::uint64_t& length) {
    length = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        char b = 0;
        if (!file.get(b)) return false;
        length |= static_cast<std::uint64_t>(
            static_cast<unsigned char>(b)) << (8 * i);
    }
    return true;
}
void writeLength(std::ofstream& file, std::uint64_t length) {
    for (unsigned int i = 0; i < 8; ++i)
        file.put(static_cast<char>((length >> (8 * i)) & 0xff));
}

bool fileLength(std::ifstream& file, std::uint64_t& size) {
    file.seekg(0, std::ios::end);
    const std::streamoff end = file.tellg();
    if (end < 0) return false;
    size = static_cast<std::uint64_t>(end);
    file.seekg(0, std::ios::beg);
    return file.good();
}

bool copyXor(std::ifstream& input, std::ofstream& output,
             const std::string& key, std::uint64_t bytes,
             std::string* error) {
    std::vector<unsigned char> buffer(kBufferSize);
    std::uint64_t position = 0;
    while (position < bytes) {
        const std::size_t amount = static_cast<std::size_t>(
            bytes - position < buffer.size() ? bytes - position : buffer.size());
        input.read(reinterpret_cast<char*>(&buffer[0]),
                   static_cast<std::streamsize>(amount));
        if (input.gcount() != static_cast<std::streamsize>(amount))
            return fail(error, "Truncated input archive");
        for (std::size_t i = 0; i < amount; ++i)
            buffer[i] ^= static_cast<unsigned char>(
                key[static_cast<std::size_t>((position + i) % key.size())]);
        output.write(reinterpret_cast<const char*>(&buffer[0]),
                     static_cast<std::streamsize>(amount));
        if (!output) return fail(error, "Output write failed");
        position += amount;
    }
    return true;
}

bool openFiles(const std::string& inputPath, const std::string& outputPath,
               const std::string& key, std::ifstream& input,
               std::ofstream& output, std::string* error) {
    if (key.empty()) return fail(error, "Empty XOR key");
    if (inputPath.empty() || outputPath.empty() || sameFile(inputPath, outputPath))
        return fail(error, "Input and output must be different files");
    input.open(inputPath.c_str(), std::ios::binary);
    if (!input) return fail(error, "Cannot open input: " + inputPath);
    // Output opens only after all basic validation has succeeded.
    output.open(outputPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!output) return fail(error, "Cannot create output: " + outputPath);
    return true;
}

bool finish(std::ofstream& output, const std::string& path,
            bool ok, std::string* error) {
    output.flush();
    if (!output) ok = fail(error, "Output flush failed");
    output.close();
    if (!ok) std::remove(path.c_str());
    return ok;
}
} // namespace

bool transformFile(const std::string& inputPath,
                   const std::string& outputPath,
                   const std::string& key,
                   std::string* error) {
    std::ifstream input;
    std::ofstream output;
    if (!openFiles(inputPath, outputPath, key, input, output, error)) return false;
    std::uint64_t length = 0;
    if (!fileLength(input, length))
        return finish(output, outputPath, fail(error, "Cannot measure input"), error);
    return finish(output, outputPath, copyXor(input, output, key, length, error), error);
}

bool encodeZipToDat(const std::string& zipPath,
                    const std::string& datPath,
                    const std::string& key,
                    std::string* error) {
    if (key.empty() || zipPath.empty() || datPath.empty() || sameFile(zipPath, datPath))
        return fail(error, "Invalid paths or empty key");
    std::ifstream input(zipPath.c_str(), std::ios::binary);
    if (!input) return fail(error, "Cannot open ZIP: " + zipPath);
    std::uint64_t length = 0;
    if (!fileLength(input, length) || length < 4)
        return fail(error, "ZIP is too small or unreadable");
    unsigned char signature[4] = {};
    input.read(reinterpret_cast<char*>(signature), 4);
    if (!input || !hasZipSignature(signature))
        return fail(error, "Input does not have a ZIP signature");
    input.seekg(0, std::ios::beg);
    std::ofstream output(datPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!output) return fail(error, "Cannot create DAT: " + datPath);
    output.write(reinterpret_cast<const char*>(kMagic), 8);
    writeLength(output, length);
    bool ok = output.good() && copyXor(input, output, key, length, error);
    if (!ok && error && error->empty()) *error = "DAT header write failed";
    return finish(output, datPath, ok, error);
}

bool decodeDatToZip(const std::string& datPath,
                    const std::string& zipPath,
                    const std::string& key,
                    std::string* error) {
    if (key.empty() || datPath.empty() || zipPath.empty() || sameFile(datPath, zipPath))
        return fail(error, "Invalid paths or empty key");
    std::ifstream input(datPath.c_str(), std::ios::binary);
    if (!input) return fail(error, "Cannot open DAT: " + datPath);
    std::uint64_t total = 0;
    if (!fileLength(input, total) || total < 20)
        return fail(error, "DAT is too small or unreadable");
    unsigned char magic[8] = {};
    input.read(reinterpret_cast<char*>(magic), 8);
    for (unsigned int i = 0; i < 8; ++i)
        if (magic[i] != kMagic[i]) return fail(error, "Invalid DAT header");
    std::uint64_t length = 0;
    if (!readLength(input, length) || length != total - 16 || length < 4)
        return fail(error, "DAT payload length mismatch");
    unsigned char first[4] = {};
    input.read(reinterpret_cast<char*>(first), 4);
    if (!input) return fail(error, "Truncated DAT payload");
    for (unsigned int i = 0; i < 4; ++i)
        first[i] ^= static_cast<unsigned char>(key[i % key.size()]);
    if (!hasZipSignature(first))
        return fail(error, "Wrong XOR key or invalid ZIP payload");
    input.seekg(16, std::ios::beg);
    std::ofstream output(zipPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!output) return fail(error, "Cannot create ZIP: " + zipPath);
    return finish(output, zipPath, copyXor(input, output, key, length, error), error);
}

} // namespace xor_archive
} // namespace hc
