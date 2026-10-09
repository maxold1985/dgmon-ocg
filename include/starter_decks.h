#pragma once
#include "card_catalog.h"
#include <string>
#include <vector>

namespace hc {
struct StarterDeck {
    std::vector<int> cards;
    int starter;
    std::string label;
    StarterDeck():starter(-1){}
};

class StarterDeckBuilder {
public:
    static bool belongsToSet(const std::string& id,int version);
    static std::vector<const CatalogCard*> cardsInSet(const CardCatalog& catalog,int version);
    static Result build(const CardCatalog& catalog,const EngineCardBridge& bridge,
                        int version,StarterDeck& deck);
};
}
