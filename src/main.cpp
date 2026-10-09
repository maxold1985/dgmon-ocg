#include "card_catalog.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
static void printCard(const hc::CatalogCard& c) {
    std::cout << c.id << " | " << c.set << " | " << c.name << " | " << c.kind;
    if (c.combatVerified) {
        std::cout << " | " << c.level << " " << c.battleType
                  << " | A=" << c.power[0] << " B=" << c.power[1] << " C=" << c.power[2]
                  << " | Lost=" << c.lost[0] << "/" << c.lost[1] << "/" << c.lost[2] << "/" << c.lost[3];
    } else std::cout << " | stats incomplete";
    std::cout << " | " << (c.isPlayableCore()?"CORE READY":"CATALOG ONLY") << "\n";
}
int main(int argc,char** argv) {
    const std::string path=argc>1?argv[1]:"data/cards.csv";
    hc::CardCatalog cards;
    hc::Result status=cards.loadCSV(path);
    if(!status.ok){std::cerr<<status.message<<"\n";return 1;}
    hc::Engine e(12345);
    hc::EngineCardBridge bridge(cards);
    std::cout<<status.message<<"; "<<bridge.eligibleCount()<<" core eligible\n";
    hc::Result registered=bridge.registerCoreCards(e);
    if(!registered.ok){std::cerr<<registered.message<<"\n";return 2;}
    std::cout<<registered.message<<"\n";
    if(argc>2 && std::string(argv[2])=="--card") {
        if(argc<4)return 3;
        const hc::CatalogCard* c=cards.find(argv[3]);
        if(!c){std::cerr<<"Card not found\n";return 4;}
        printCard(*c);
        return 0;
    }
    if(argc>2 && std::string(argv[2])=="--set") {
        if(argc<4)return 3;
        std::vector<const hc::CatalogCard*> v=cards.bySet(argv[3]);
        for(size_t i=0;i<v.size();++i)printCard(*v[i]);
        std::cout<<"Total="<<v.size()<<"\n";
        return 0;
    }
    if(argc>2 && std::string(argv[2])=="--demo") {
        // Exactly 30 cards, 10 distinct rookie card names, 3 copies each.
        const char* names[]={"St-1","St-3","St-5","St-7","St-9","St-11","St-13","St-18","St-23","St-24"};
        std::vector<std::string> ids;
        for(int i=0;i<10;++i)for(int k=0;k<3;++k)ids.push_back(names[i]);
        const std::vector<int> deck=bridge.buildDeck(ids);
        if(deck.size()!=30){std::cerr<<"Demo deck has unverified cards\n";return 5;}
        hc::Result r=e.start(deck,deck,bridge.numberToInternal("St-1"),bridge.numberToInternal("St-5"),0);
        if(!r.ok){std::cerr<<r.message<<"\n";return 6;}
        std::cout<<"Setup: "<<r.message<<"\n";
        for(int round=0;round<3 && e.phase()!=hc::Phase::Finished;++round) {
            const int first=e.firstPlayer();
            if(!e.commitPreparation(first).ok||!e.commitPreparation(1-first).ok)return 7;
            if(!e.evolve(first).ok||!e.evolve(1-first).ok)return 8;
            if(!e.resolveBattle().ok)return 9;
            std::cout<<"Round "<<e.round()<<" A="<<e.lastPower(0)<<" B="<<e.lastPower(1)<<" winner="<<e.winner()<<"\n";
            if(!e.resolvePoints().ok)return 10;
            std::cout<<"Points "<<e.getPlayer(0).points<<" - "<<e.getPlayer(1).points<<"\n";
        }
    } else {
        const hc::CatalogCard* a=cards.find("St-1");
        const hc::CatalogCard* b=cards.find("Bo-1");
        if(a)printCard(*a);
        if(b)printCard(*b);
        std::cout<<"Commands: <csv> --card St-1 | --set \"Booster 1\" | --demo\n";
    }
    return 0;
}
