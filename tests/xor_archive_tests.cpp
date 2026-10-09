#include "xor_archive.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::string bytes(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
}
int main() {
    const char* input = "hc_xor_test_original.zip";
    const char* dat = "hc_xor_test_encoded.dat";
    const char* output = "hc_xor_test_recovered.zip";
    const char* bad = "hc_xor_test_bad.zip";
    const char* raw = "hc_xor_test_raw.dat";
    const char* twice = "hc_xor_test_twice.zip";
    std::vector<unsigned char> original;
    original.push_back('P'); original.push_back('K');
    original.push_back(3); original.push_back(4);
    for (int i = 0; i < 180000; ++i)
        original.push_back(static_cast<unsigned char>(i % 256));
    {
        std::ofstream file(input, std::ios::binary);
        file.write(reinterpret_cast<const char*>(&original[0]),
                   static_cast<std::streamsize>(original.size()));
    }
    std::string error;
    assert(hc::xor_archive::encodeZipToDat(input, dat, "My-XOR-key-123", &error));
    assert(bytes(dat).size() == original.size() + 16);
    assert(!hc::xor_archive::decodeDatToZip(dat, bad, "wrong", &error));
    {
        std::ifstream shouldNotExist(bad, std::ios::binary);
        assert(!shouldNotExist);
    }
    assert(hc::xor_archive::decodeDatToZip(dat, output, "My-XOR-key-123", &error));
    assert(bytes(input) == bytes(output));
    assert(hc::xor_archive::transformFile(input, raw, "xyz", &error));
    assert(hc::xor_archive::transformFile(raw, twice, "xyz", &error));
    assert(bytes(input) == bytes(twice));
    assert(!hc::xor_archive::encodeZipToDat(input, input, "key", &error));
    assert(!hc::xor_archive::encodeZipToDat(input, dat, "", &error));
    std::remove(input); std::remove(dat); std::remove(output);
    std::remove(bad); std::remove(raw); std::remove(twice);
    return 0;
}
