#include "card_catalog.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
namespace hc {
namespace {
std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted=false;
    for (size_t i=0;i<line.size();++i) {
        const char c=line[i];
        if (quoted && c=='"' && i+1<line.size() && line[i+1]=='"') {field+='"';++i;}
        else if (c=='"') quoted=!quoted;
        else if (!quoted && c==',') {fields.push_back(field);field.clear();}
        else field+=c;
    }
    if (quoted) return std::vector<std::string>();
    fields.push_back(field);
    return fields;
}
bool isStarterCard(const std::string& id) {
    if(id.size()<4 || id.compare(0,3,"St-")!=0)return false;
    int number=0;
    for(size_t i=3;i<id.size();++i) {
        if(id[i]<'0'||id[i]>'9')return false;
        number=number*10+(id[i]-'0');
        if(number>111)return false;
    }
    return number>=1;
}
int positive(const std::string& text) {
    if(text.empty()) return -1;
    std::istringstream s(text);
    int x=-1;
    char junk=0;
    if(!(s>>x)||x<0||(s>>junk)) return -1;
    return x;
}
int parseLevel(const std::string& l) {
    if(l=="III")return 3;
    if(l=="IV")return 4;
    if(l=="Perfect")return 5;
    if(l=="Ultimate")return 6;
    return 0;
}
int parseAttack(const std::string& text) {
    if(text=="A")return 0;
    if(text=="B")return 1;
    if(text=="C")return 2;
    return -1;
}
}
bool CatalogCard::isPlayableCore() const {
    if(kind!="Digimon"||!combatVerified||parseLevel(level)==0||parseAttack(battleType)<0)return false;
    // Special/unknown effects and unsupported evolution requirements stay locked.
    if(effectStatus!="none"&&effectStatus!="passive_only")return false;
    if(parseLevel(level)>3&&evolutionRequirements.empty())return false;
    if(evolutionRequirements.find("WP")!=std::string::npos||evolutionRequirements.find("JOGRESS")!=std::string::npos)return false;
    if(parseLevel(level)>3) {
        std::istringstream lines(evolutionRequirements);
        std::string token, firstCost;
        while(std::getline(lines, token, ';')) {
            const size_t sep=token.find(':');
            if(sep==std::string::npos)return false;
            const std::string cost=token.substr(sep+1);
            if(cost.empty()||cost.find_first_not_of("OX")!=std::string::npos)return false;
            if(firstCost.empty())firstCost=cost;
            // The old engine stores one shared evolution cost per card.
            if(std::count(cost.begin(),cost.end(),'O')!=std::count(firstCost.begin(),firstCost.end(),'O') ||
               std::count(cost.begin(),cost.end(),'X')!=std::count(firstCost.begin(),firstCost.end(),'X'))return false;
        }
    }
    return true;
}
Result CardCatalog::loadCSV(const std::string& filename) {
    std::ifstream f(filename.c_str(), std::ios::binary);
    if(!f)return {false,"Cannot open CSV: "+filename};
    std::string text((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
    return loadCSVText(text);
}
Result CardCatalog::loadCSVText(const std::string& text) {
    std::istringstream f(text);
    std::string line;
    if(!std::getline(f,line))return {false,"Empty CSV"};
    if(!line.empty()&&line[line.size()-1]=='\r')line.resize(line.size()-1);
    const std::vector<std::string> header=splitCSV(line);
    const std::string required[]={"id","set","name_en","name_jp","kind","level","battle_type","attack_a","attack_b","attack_c","cancel_target","lost_iii","lost_iv","lost_perfect","lost_ultimate","evolution_requirements","effect_status","source"};
    // Support both the original 18-column CSV and enriched 33-column database.
    if(header.size()!=18&&header.size()!=33)return {false,"Wrong CSV header width"};
    for(int j=0;j<18;++j)if(header[j]!=required[j])return {false,"Wrong CSV header: "+header[j]};
    if(header.size()==33) {
        const std::string metadata[]={
            "digimon_type","attribute","field_code","frame","option_type",
            "attack_a_name","attack_b_name","attack_c_name","special_ability",
            "printed_bonus","image_file","source_set","details_source",
            "verification_level","notes"
        };
        for(int j=0;j<15;++j)
            if(header[18+j]!=metadata[j])return {false,"Wrong metadata header: "+header[18+j]};
    }
    std::map<std::string,CatalogCard> incoming;
    int rowNum=1;
    while(std::getline(f,line)) {
        ++rowNum;
        if(!line.empty()&&line[line.size()-1]=='\r')line.resize(line.size()-1);
        if(line.empty())continue;
        const std::vector<std::string> r=splitCSV(line);
        if(r.size()!=header.size()){std::ostringstream o;o<<"Bad CSV row "<<rowNum;return {false,o.str()};}
        CatalogCard c;
        c.id=r[0];c.set=r[1];c.name=r[2];c.japanese=r[3];c.kind=r[4];c.level=r[5];c.battleType=r[6];
        for(int j=0;j<3;++j)c.power[j]=positive(r[7+j]);
        c.cancelTarget=r[10];
        for(int j=0;j<4;++j)c.lost[j]=positive(r[11+j]);
        c.evolutionRequirements=r[15];c.effectStatus=r[16];c.source=r[17];
        if(header.size()==33) {
            c.digimonType=r[18];c.attribute=r[19];c.fieldCode=r[20];
            c.frame=r[21];c.optionType=r[22];
            for(int j=0;j<3;++j)c.attackNames[j]=r[23+j];
            c.specialAbility=r[26];
            c.printedBonus=positive(r[27]);
            c.imageFile=r[28];c.sourceSet=r[29];c.detailsSource=r[30];
            c.verificationLevel=r[31];c.notes=r[32];
        }
        c.combatVerified=(parseLevel(c.level)!=0&&parseAttack(c.battleType)>=0);
        for(int j=0;j<3;++j)c.combatVerified=c.combatVerified&&(c.power[j]>=0);
        for(int j=0;j<4;++j)c.combatVerified=c.combatVerified&&(c.lost[j]>=0);
        c.combatVerified=c.combatVerified&&(c.cancelTarget=="A"||c.cancelTarget=="B"||c.cancelTarget=="C"||c.cancelTarget=="none");
        if(c.id.empty()||c.name.empty()||incoming.count(c.id))return {false,"Duplicate/empty card ID: "+c.id};
        if(isStarterCard(c.id))incoming[c.id]=c;
    }
    if(incoming.empty())return {false,"No cards"};
    cards_.swap(incoming);
    std::ostringstream o;o<<"Loaded "<<cards_.size()<<" card IDs";
    return {true,o.str()};
}
const CatalogCard* CardCatalog::find(const std::string& cardNumber) const {
    std::map<std::string,CatalogCard>::const_iterator i=cards_.find(cardNumber);
    return i==cards_.end()?0:&i->second;
}
std::vector<const CatalogCard*> CardCatalog::all() const {
    std::vector<const CatalogCard*> v;
    for(std::map<std::string,CatalogCard>::const_iterator i=cards_.begin();i!=cards_.end();++i)v.push_back(&i->second);
    return v;
}
std::vector<const CatalogCard*> CardCatalog::bySet(const std::string& setName) const {
    std::vector<const CatalogCard*> v;
    for(std::map<std::string,CatalogCard>::const_iterator i=cards_.begin();i!=cards_.end();++i)
        if(i->second.set==setName)v.push_back(&i->second);
    return v;
}
std::vector<const CatalogCard*> CardCatalog::byName(const std::string& text) const {
    std::vector<const CatalogCard*> v;
    for(std::map<std::string,CatalogCard>::const_iterator i=cards_.begin();i!=cards_.end();++i)
        if(i->second.name.find(text)!=std::string::npos||i->second.japanese.find(text)!=std::string::npos)v.push_back(&i->second);
    return v;
}
EngineCardBridge::EngineCardBridge(const CardCatalog& catalog):catalog_(catalog) {
    const std::vector<const CatalogCard*> cards=catalog_.all();
    for(size_t k=0;k<cards.size();++k) {
        const int internal=(int)k+1;
        toId_[cards[k]->id]=internal;
        toNumber_[internal]=cards[k]->id;
        // Only three verified battle Plug-Ins have an implementation so far.
        if(cards[k]->isPlayableCore() ||
           cards[k]->id=="St-49"||cards[k]->id=="St-50"||cards[k]->id=="St-51")
            allowed_[cards[k]->id]=true;
    }
}
int EngineCardBridge::numberToInternal(const std::string& number) const {
    if(!allowed_.count(number))return -1;
    std::map<std::string,int>::const_iterator i=toId_.find(number);
    return i==toId_.end()?-1:i->second;
}
std::string EngineCardBridge::internalToNumber(int n) const {
    std::map<int,std::string>::const_iterator i=toNumber_.find(n);
    return i==toNumber_.end()?std::string():i->second;
}
Result EngineCardBridge::registerCoreCards(Engine& engine) const {
    const std::vector<const CatalogCard*> cards=catalog_.all();
    size_t ready=0;
    for(size_t i=0;i<cards.size();++i) {
        const CatalogCard& c=*cards[i];
        if(!allowed_.count(c.id))continue;
        Card out;
        out.id=toId_.find(c.id)->second;
        out.name=c.name;
        if(c.kind=="Option") {
            // St-49 A, St-50 C, St-51 B: effect documented on Wikimon.
            out.kind=Kind::Option;
            out.optionAttackOverride=c.id=="St-49"?0:(c.id=="St-50"?2:1);
            engine.registerCard(out);
            ++ready;
            continue;
        }
        out.kind=Kind::Digimon;
        out.level=(Level)parseLevel(c.level);
        out.battleType=(Attack)parseAttack(c.battleType);
        for(int n=0;n<3;++n)out.power[n]=c.power[n];
        for(int n=0;n<4;++n)out.lostPoints[n]=c.lost[n];
        out.cancelAttack=parseAttack(c.cancelTarget);
        out.regularCost=0;out.irregularCost=0;
        if(parseLevel(c.level)>3) {
            // Supported only when every published prerequisite is a standard ●/× path.
            // Only parse source-verified name:OO;name:XX;... .
            std::istringstream stream(c.evolutionRequirements);
            std::string token;
            while(std::getline(stream,token,';')) {
                const size_t sep=token.find(':');
                if(sep==std::string::npos)return {false,"Invalid evolution metadata: "+c.id};
                const std::string from=token.substr(0,sep);
                const std::string costs=token.substr(sep+1);
                int circles=0,crosses=0;
                for(size_t x=0;x<costs.size();++x) {
                    if(costs[x]=='O')++circles;
                    else if(costs[x]=='X')++crosses;
                    else return {false,"Unsupported evolution requirement: "+c.id};
                }
                // Existing engine models one common cost for all paths. Reject cards with
                // path-dependent costs rather than incorrectly allowing illegal evolutions.
                if(!out.evolvesFrom.empty()&&(circles!=out.regularCost||crosses!=out.irregularCost)) {
                    out.requiresSpecialEvolution=true;
                    break;
                }
                out.regularCost=circles;out.irregularCost=crosses;
                for(size_t a=0;a<cards.size();++a)if(cards[a]->name==from&&allowed_.count(cards[a]->id))
                    out.evolvesFrom.push_back(toId_.find(cards[a]->id)->second);
            }
        }
        if(out.requiresSpecialEvolution)continue;
        engine.registerCard(out);
        ++ready;
    }
    std::ostringstream s;s<<ready<<" card entries registered in battle engine";
    return {true,s.str()};
}
std::vector<int> EngineCardBridge::buildDeck(const std::vector<std::string>& numbers) const {
    std::vector<int> v;
    for(size_t i=0;i<numbers.size();++i) {
        const int n=numberToInternal(numbers[i]);
        if(n<0)return std::vector<int>();
        v.push_back(n);
    }
    return v;
}
}
