#include "dat_archive.h"
#include "xor_archive.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#ifdef NDEBUG
#undef NDEBUG
#endif
// A minimal ZIP_STORED with one entry, generated in memory for the test.
static void u16(std::vector<unsigned char>& b,unsigned v) {
    b.push_back((unsigned char)v);b.push_back((unsigned char)(v>>8));
}
static void u32(std::vector<unsigned char>& b,unsigned v) {
    u16(b,v);u16(b,v>>16);
}
static unsigned crc(const std::string& s) {
    unsigned c=0xffffffffu;
    for(std::size_t i=0;i<s.size();++i) {
        c^=(unsigned char)s[i];
        for(int bit=0;bit<8;++bit)c=(c>>1)^(0xedb88320u & (0u-(c&1u)));
    }
    return ~c;
}
int main() {
    const std::string name="starter_cards.csv",contents="id,name\nSt-1,Agumon\n";
    std::vector<unsigned char> zip;
    u32(zip,0x04034b50u);u16(zip,20);u16(zip,0);u16(zip,0);
    u16(zip,0);u16(zip,0);u32(zip,crc(contents));
    u32(zip,(unsigned)contents.size());u32(zip,(unsigned)contents.size());
    u16(zip,(unsigned)name.size());u16(zip,0);
    zip.insert(zip.end(),name.begin(),name.end());
    zip.insert(zip.end(),contents.begin(),contents.end());
    const unsigned central=(unsigned)zip.size();
    u32(zip,0x02014b50u);u16(zip,20);u16(zip,20);u16(zip,0);u16(zip,0);
    u16(zip,0);u16(zip,0);u32(zip,crc(contents));
    u32(zip,(unsigned)contents.size());u32(zip,(unsigned)contents.size());
    u16(zip,(unsigned)name.size());u16(zip,0);u16(zip,0);
    u16(zip,0);u16(zip,0);u32(zip,0);u32(zip,0);
    zip.insert(zip.end(),name.begin(),name.end());
    const unsigned dirSize=(unsigned)zip.size()-central;
    u32(zip,0x06054b50u);u16(zip,0);u16(zip,0);u16(zip,1);u16(zip,1);
    u32(zip,dirSize);u32(zip,central);u16(zip,0);
    const char* zipPath="hc_dat_memory_test.zip";
    const char* datPath="hc_dat_memory_test.dat";
    {
        std::ofstream f(zipPath,std::ios::binary);
        f.write((const char*)&zip[0],(std::streamsize)zip.size());
    }
    std::string error;
    assert(hc::xor_archive::encodeZipToDat(zipPath,datPath,"secret",&error));
    hc::DatArchive archive;
    assert(!archive.open(datPath,"wrong",&error));
    assert(archive.open(datPath,"secret",&error));
    std::string text;
    assert(archive.readText(name,text));
    assert(text==contents);
    assert(!archive.readText("not_found.jpg",text));
    std::remove(zipPath);std::remove(datPath);
    return 0;
}
