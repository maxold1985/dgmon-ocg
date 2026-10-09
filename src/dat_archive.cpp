#include "dat_archive.h"
#include <fstream>
#include <iterator>
#include <limits>

namespace hc {
namespace {
std::uint16_t u16(const std::vector<unsigned char>& a,std::size_t p) {
    return static_cast<std::uint16_t>(a[p] | (static_cast<unsigned int>(a[p+1])<<8));
}
std::uint32_t u32(const std::vector<unsigned char>& a,std::size_t p) {
    return static_cast<std::uint32_t>(a[p]) |
           (static_cast<std::uint32_t>(a[p+1])<<8) |
           (static_cast<std::uint32_t>(a[p+2])<<16) |
           (static_cast<std::uint32_t>(a[p+3])<<24);
}
bool range(std::size_t start,std::size_t length,std::size_t total) {
    return start<=total && length<=total-start;
}
bool failure(std::string* e,const char* msg) {
    if(e)*e=msg;
    return false;
}
std::uint32_t crc32(const unsigned char* bytes,std::size_t length) {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<length;++i) {
        crc^=bytes[i];
        for(int bit=0;bit<8;++bit)
            crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
bool zipSignature(const std::vector<unsigned char>& bytes) {
    return bytes.size()>=4&&bytes[0]=='P'&&bytes[1]=='K'&&
           ((bytes[2]==3&&bytes[3]==4)||(bytes[2]==5&&bytes[3]==6));
}
}
bool DatArchive::open(const std::string& path,const std::string& key,std::string* error) {
    zip_.clear();entries_.clear();
    if(key.empty())return failure(error,"Empty XOR key");
    std::ifstream file(path.c_str(),std::ios::binary);
    if(!file)return failure(error,"Cannot open DAT");
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
    const unsigned char magic[8]={'H','C','X','O','R','0','1','\n'};
    if(bytes.size()<38)return failure(error,"DAT too small");
    for(unsigned int i=0;i<8;++i)
        if(bytes[i]!=magic[i])return failure(error,"Invalid DAT magic");
    std::uint64_t length=0;
    for(unsigned int i=0;i<8;++i)length|=static_cast<std::uint64_t>(bytes[8+i])<<(8*i);
    if(length!=bytes.size()-16)return failure(error,"DAT length mismatch");
    zip_.assign(bytes.begin()+16,bytes.end());
    for(std::size_t i=0;i<zip_.size();++i)
        zip_[i]^=static_cast<unsigned char>(key[i%key.size()]);
    if(!zipSignature(zip_)) {
        zip_.clear();
        return failure(error,"Wrong XOR key or invalid ZIP");
    }
    // Search for the end of central directory in the last 65557 bytes.
    std::size_t eocd=zip_.size();
    const std::size_t first=zip_.size()>65557?zip_.size()-65557:0;
    for(std::size_t i=zip_.size()-22;;--i) {
        if(u32(zip_,i)==0x06054b50u && range(i,22,zip_.size()) &&
           i+22+u16(zip_,i+20)==zip_.size()) {eocd=i;break;}
        if(i==first)break;
    }
    if(eocd==zip_.size()){zip_.clear();return failure(error,"Missing ZIP central directory");}
    if(u16(zip_,eocd+4)!=0||u16(zip_,eocd+6)!=0) {
        zip_.clear();return failure(error,"Multidisk ZIP unsupported");
    }
    const std::size_t count=u16(zip_,eocd+10);
    const std::size_t dirSize=u32(zip_,eocd+12),dirOffset=u32(zip_,eocd+16);
    if(!range(dirOffset,dirSize,eocd)) {
        zip_.clear();return failure(error,"Invalid ZIP directory range");
    }
    std::size_t pos=dirOffset;
    for(std::size_t i=0;i<count;++i) {
        if(!range(pos,46,eocd)||u32(zip_,pos)!=0x02014b50u) {
            zip_.clear();entries_.clear();return failure(error,"Invalid ZIP directory entry");
        }
        const unsigned int flags=u16(zip_,pos+8),method=u16(zip_,pos+10);
        const std::uint32_t crc=u32(zip_,pos+16);
        const std::uint32_t compressed=u32(zip_,pos+20),uncompressed=u32(zip_,pos+24);
        const std::size_t nameLen=u16(zip_,pos+28),extra=u16(zip_,pos+30),comment=u16(zip_,pos+32);
        const std::size_t local=u32(zip_,pos+42);
        if(!range(pos+46,nameLen+extra+comment,eocd)) {
            zip_.clear();entries_.clear();return failure(error,"Truncated ZIP directory");
        }
        const std::string name(reinterpret_cast<const char*>(&zip_[pos+46]),nameLen);
        if(name.empty()||name.find("..")!=std::string::npos||
           name[0]=='/'||name.find('\\')!=std::string::npos||
           (name.size()>1&&name[1]==':')) {
            zip_.clear();entries_.clear();return failure(error,"Unsafe ZIP entry name");
        }
        if(method!=0||compressed!=uncompressed||(flags&1)!=0) {
            zip_.clear();entries_.clear();
            return failure(error,"ZIP must use ZIP_STORED (no compression or encryption)");
        }
        if(!range(local,30,zip_.size())||u32(zip_,local)!=0x04034b50u) {
            zip_.clear();entries_.clear();return failure(error,"Invalid ZIP local header");
        }
        const std::size_t offset=local+30+u16(zip_,local+26)+u16(zip_,local+28);
        if(!range(offset,compressed,zip_.size())) {
            zip_.clear();entries_.clear();return failure(error,"Invalid ZIP entry bounds");
        }
        if(name[name.size()-1]!='/') {
            if(entries_.count(name)) {
                zip_.clear();entries_.clear();return failure(error,"Duplicate ZIP entry");
            }
            Entry entry={static_cast<std::uint32_t>(offset),uncompressed,crc};
            entries_[name]=entry;
        }
        pos+=46+nameLen+extra+comment;
    }
    if(entries_.empty()) {
        zip_.clear();return failure(error,"No files in ZIP");
    }
    return true;
}
bool DatArchive::readFile(const std::string& name,std::vector<unsigned char>& output) const {
    output.clear();
    std::map<std::string,Entry>::const_iterator it=entries_.find(name);
    if(it==entries_.end())return false;
    const Entry& entry=it->second;
    if(!range(entry.offset,entry.size,zip_.size()))return false;
    const unsigned char* data=&zip_[entry.offset];
    if(crc32(data,entry.size)!=entry.crc)return false;
    output.assign(data,data+entry.size);
    return true;
}
bool DatArchive::readText(const std::string& name,std::string& output) const {
    std::vector<unsigned char> data;
    if(!readFile(name,data))return false;
    output.assign(data.begin(),data.end());
    return true;
}
bool DatArchive::contains(const std::string& name) const {
    return entries_.find(name)!=entries_.end();
}
}
