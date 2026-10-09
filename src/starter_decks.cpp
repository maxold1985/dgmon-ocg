#include "starter_decks.h"
#include <set>
#include <sstream>
#include <cstdlib>

namespace hc {
namespace {
int cardNumber(const std::string& id) {
    if(id.size()<4||id.compare(0,3,"St-")!=0)return -1;
    int number=0;
    for(size_t i=3;i<id.size();++i) {
        if(id[i]<'0'||id[i]>'9')return -1;
        number=number*10+(id[i]-'0');
        if(number>111)return -1;
    }
    return number;
}
}
bool StarterDeckBuilder::belongsToSet(const std::string& id,int version) {
    const int n=cardNumber(id);
    if(version==1)return n>=1&&n<=60;
    if(version==2) {
        if(n>=61&&n<=111)return true;
        return n==1||n==3||n==5||n==7||n==11||n==13||
               n==49||n==50||n==51;
    }
    return false;
}
std::vector<const CatalogCard*> StarterDeckBuilder::cardsInSet(
    const CardCatalog& catalog,int version) {
    std::vector<const CatalogCard*> result;
    const std::vector<const CatalogCard*> all=catalog.all();
    for(size_t i=0;i<all.size();++i)
        if(belongsToSet(all[i]->id,version))result.push_back(all[i]);
    return result;
}
Result StarterDeckBuilder::build(const CardCatalog& catalog,
                                const EngineCardBridge& bridge,
                                int version,StarterDeck& output) {
    if(version!=1&&version!=2)return {false,"Unsupported starter set"};
    StarterDeck temp;
    temp.label=version==1?"Starter Ver. 1":"Starter Ver. 2";
    const std::vector<const CatalogCard*> pool=cardsInSet(catalog,version);
    std::set<std::string> distinctNames;
    std::vector<std::string> digimon;
    // Preserve cards' numeric order and use only reviewed battle entries.
    for(size_t i=0;i<pool.size()&&digimon.size()<9;++i) {
        const CatalogCard& c=*pool[i];
        if(c.kind!="Digimon"||!c.isPlayableCore()||
           bridge.numberToInternal(c.id)<0)continue;
        if(distinctNames.insert(c.name).second)digimon.push_back(c.id);
    }
    if(digimon.size()!=9) {
        std::ostringstream o;
        o<<temp.label<<": need nine distinct verified Digimon names; found "
         <<digimon.size();
        return {false,o.str()};
    }
    const std::string preferred=version==1?"St-1":"St-62";
    bool hasStarter=false;
    for(size_t i=0;i<digimon.size();++i)if(digimon[i]==preferred)hasStarter=true;
    if(!hasStarter)return {false,"Required verified Level III starter is missing: "+preferred};
    const int option=bridge.numberToInternal("St-49");
    if(option<0)return {false,"St-49 option not implemented"};
    for(size_t i=0;i<digimon.size();++i) {
        const int id=bridge.numberToInternal(digimon[i]);
        for(int n=0;n<3;++n)temp.cards.push_back(id);
    }
    for(int n=0;n<3;++n)temp.cards.push_back(option);
    temp.starter=bridge.numberToInternal(preferred);
    if(temp.cards.size()!=30 || temp.starter<0)return {false,"Invalid starter deck construction"};
    output=temp;
    return {true,temp.label+" ready: 30 cards, including St-49"};
}
}
