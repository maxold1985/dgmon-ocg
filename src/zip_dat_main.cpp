#include "xor_archive.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage:\n"
                  << "  hc_zip_dat encode <input.zip> <output.dat> <xor-key>\n"
                  << "  hc_zip_dat decode <input.dat> <output.zip> <xor-key>\n"
                  << "  hc_zip_dat xor <input-file> <output-file> <xor-key>\n";
        return 2;
    }
    const std::string action = argv[1];
    std::string error;
    bool ok = false;
    if (action == "encode")
        ok = hc::xor_archive::encodeZipToDat(argv[2], argv[3], argv[4], &error);
    else if (action == "decode")
        ok = hc::xor_archive::decodeDatToZip(argv[2], argv[3], argv[4], &error);
    else if (action == "xor")
        ok = hc::xor_archive::transformFile(argv[2], argv[3], argv[4], &error);
    else {
        std::cerr << "Unknown operation: " << action << "\n";
        return 2;
    }
    if (!ok) {
        std::cerr << "ERROR: " << error << "\n";
        return 1;
    }
    std::cout << "OK: " << argv[3] << "\n";
    return 0;
}
