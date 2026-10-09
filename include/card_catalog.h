#pragma once
#include "engine.h"
#include <map>
#include <string>
#include <vector>
namespace hc {
struct CatalogCard {
    std::string id, set, name, japanese, kind, level, battleType;
    std::string evolutionRequirements, effectStatus, source;
    std::string cancelTarget;
    int power[3], lost[4];
    bool combatVerified;
    CatalogCard() : combatVerified(false) {
        for (int i=0;i<3;++i) power[i]=-1;
        for (int i=0;i<4;++i) lost[i]=-1;
    }
    bool isPlayableCore() const;
};
class CardCatalog {
public:
    Result loadCSV(const std::string& filename);
    const CatalogCard* find(const std::string& cardNumber) const;
    std::vector<const CatalogCard*> bySet(const std::string& setName) const;
    std::vector<const CatalogCard*> byName(const std::string& text) const;
    std::vector<const CatalogCard*> all() const;
    size_t size() const { return cards_.size(); }
private:
    std::map<std::string, CatalogCard> cards_;
};
// Stable, collision-free mapping within a loaded catalog. C++11 VS2013 compatible.
class EngineCardBridge {
public:
    explicit EngineCardBridge(const CardCatalog& catalog);
    Result registerCoreCards(Engine& engine) const;
    int numberToInternal(const std::string& number) const;
    std::string internalToNumber(int internalId) const;
    std::vector<int> buildDeck(const std::vector<std::string>& numbers) const;
    size_t eligibleCount() const { return allowed_.size(); }
private:
    const CardCatalog& catalog_;
    std::map<std::string,int> toId_;
    std::map<int,std::string> toNumber_;
    std::map<std::string,bool> allowed_;
};
}
