#include "card_catalog.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
int main(int argc, char** argv) {
    if(argc!=2)return 9;
    hc::CardCatalog c;
    hc::Result loaded=c.loadCSV(argv[1]);
    assert(loaded.ok);
    assert(c.size()==283);
    assert(c.bySet("Starter Ver. 1").size()==60);
    assert(c.bySet("Booster 1").size()==54);
    assert(c.bySet("Starter Ver. 7").size()==80);
    assert(c.bySet("Booster 15").size()==50);
    assert(c.bySet("Starter Ver. 2").size()==39);
    const hc::CatalogCard* st1=c.find("St-1");
    const hc::CatalogCard* st2=c.find("St-2");
    const hc::CatalogCard* bo1=c.find("Bo-1");
    const hc::CatalogCard* bo704=c.find("Bo-704");
    assert(st1&&st2&&bo1&&bo704);
    assert(st1->combatVerified&&st1->power[0]==360&&st1->lost[0]==10);
    assert(st2->combatVerified&&st2->power[0]==440&&st2->lost[0]==20);
    assert(bo1->combatVerified&&bo1->lost[1]==20&& !bo1->isPlayableCore());
    assert(bo704->power[2]==150 && !bo704->combatVerified);
    assert(!c.find("St-27")->isPlayableCore()); // alternate irregular evolution costs
    hc::EngineCardBridge bridge(c);hc::Engine engine(9001);
    assert(bridge.registerCoreCards(engine).ok);
    assert(bridge.numberToInternal("St-1")>0);
    assert(bridge.numberToInternal("St-49")==-1); // effect not implemented
    assert(bridge.numberToInternal("Bo-1")==-1); // WP40 effect unsupported
    assert(bridge.numberToInternal("St-27")==-1); // alternative evo cost unsupported
    std::vector<std::string> ids;
    const char* good[]={"St-1","St-3","St-5","St-7","St-9","St-11","St-13","St-18","St-23","St-24"};
    for(int i=0;i<10;++i)for(int n=0;n<3;++n)ids.push_back(good[i]);
    std::vector<int> valid=bridge.buildDeck(ids);
    assert(valid.size()==30);
    assert(engine.start(valid,valid,bridge.numberToInternal("St-1"),bridge.numberToInternal("St-5"),0).ok);
    assert(engine.getPlayer(0).hand.size()==6);
    assert(engine.getPlayer(0).deck.size()==22);
    assert(engine.commitPreparation(0).ok);
    assert(engine.commitPreparation(1).ok);
    assert(engine.evolve(0).ok);
    assert(engine.evolve(1).ok);
    assert(engine.resolveBattle().ok);
    assert(engine.lastPower(0)==230 && engine.lastPower(1)==350);
    assert(engine.resolvePoints().ok);
    assert(engine.getPlayer(0).points==90);
    // Deck 30 cards with 4 copies of same card name must fail.
    hc::Engine illegal(5);
    assert(bridge.registerCoreCards(illegal).ok);
    valid[3]=bridge.numberToInternal("St-1");
    assert(!illegal.start(valid,valid,bridge.numberToInternal("St-1"),bridge.numberToInternal("St-5"),0).ok);
    std::cout << "Core and catalog tests PASS: " << c.size() << " IDs\n";
    return 0;
}
