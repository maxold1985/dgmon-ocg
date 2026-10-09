#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hc {
// Loads HCXOR01 DAT into RAM and reads ZIP_STORED entries without extracting files.
// ZIP_DEFLATED entries are rejected explicitly. ZIP64/multidisk not supported.
class DatArchive {
public:
    bool open(const std::string& path,const std::string& key,std::string* error=0);
    bool readFile(const std::string& name,std::vector<unsigned char>& output) const;
    bool readText(const std::string& name,std::string& output) const;
    bool contains(const std::string& name) const;
    bool isOpen() const { return !entries_.empty(); }
private:
    struct Entry { std::uint32_t offset,size,crc; };
    std::vector<unsigned char> zip_;
    std::map<std::string,Entry> entries_;
};
}
