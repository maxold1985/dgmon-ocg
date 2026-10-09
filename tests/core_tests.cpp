#include "card_catalog.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>\n#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
int main(int argc, char** argv) {
    if(argc!=2)return 9;
    hc::CardCatalog c;
    hc::Result loaded=c.loadCSV(argv[1]);
    assert(loaded.ok);
    assert(c.size() > 0);
    assert(c.size() <= 300);
    for (const hc::CatalogCard* card : c.all()) {
        assert(card->id.compare(0, 3, "Bo-") == 0);
        const int n = std::atoi(card->id.c_str() + 3);
        assert(n >= 1 && n <= 300);
    }
    assert(c.find("Bo-1") != 0);
    assert(c.find("St-1") == 0);
    assert(c.find("Bo-704") == 0);
    hc::EngineCardBridge bridge(c);
    hc::Engine engine(9001);
    assert(bridge.registerCoreCards(engine).ok);
    assert(bridge.numberToInternal("St-1") == -1);
    assert(bridge.numberToInternal("Bo-704") == -1);
    std::cout << "Core and catalog tests PASS: " << c.size() << " IDs\n";
    return 0;
}
