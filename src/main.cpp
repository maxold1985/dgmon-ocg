#include "starter_decks.h"
#include "auto_player.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static void printCard(const hc::CatalogCard& c) {
    std::cout<<c.id<<" | "<<c.set<<" | "<<c.name<<" | "<<c.kind;
    if(c.combatVerified) {
        std::cout<<" | Level "<<c.level<<" | Battle "<<c.battleType
                 <<" | A="<<c.power[0]<<" B="<<c.power[1]<<" C="<<c.power[2]
                 <<" | Lost="<<c.lost[0]<<"/"<<c.lost[1]<<"/"<<c.lost[2]<<"/"<<c.lost[3];
    } else if(c.kind=="Digimon")
        std::cout<<" | A="<<c.power[0]<<" B="<<c.power[1]<<" C="<<c.power[2]
                 <<" | battle rules pending verification";
    if(!c.digimonType.empty())
        std::cout<<" | Type="<<c.digimonType;
    if(!c.attribute.empty())
        std::cout<<" | Attribute="<<c.attribute;
    if(!c.fieldCode.empty())
        std::cout<<" | Field="<<c.fieldCode;
    if(!c.japanese.empty())
        std::cout<<" | JP="<<c.japanese;
    if(!c.optionType.empty())
        std::cout<<" | Option="<<c.optionType;
    if(c.printedBonus>=0)
        std::cout<<" | Printed bonus=+"<<c.printedBonus;
    if(!c.specialAbility.empty())
        std::cout<<" | Ability="<<c.specialAbility;
    if(!c.attackNames[0].empty())
        std::cout<<" | Attacks="<<c.attackNames[0]<<"/"<<c.attackNames[1]<<"/"<<c.attackNames[2];
    std::cout<<" | "<<c.effectStatus
             <<" | Evidence="<<c.verificationLevel
             <<" | "<<c.source<<"\n";
}
int main(int argc,char** argv) {
    const std::string path=argc>1?argv[1]:"data/starter_cards.csv";
    hc::CardCatalog cards;
    hc::Result status=cards.loadCSV(path);
    if(!status.ok){std::cerr<<status.message<<"\n";return 1;}
    hc::EngineCardBridge bridge(cards);
    hc::Engine engine(12345);
    std::cout<<status.message<<"; "<<bridge.eligibleCount()
             <<" programmed cards (includes three battle Plug-Ins)\n";
    hc::Result registered=bridge.registerCoreCards(engine);
    if(!registered.ok){std::cerr<<registered.message<<"\n";return 2;}
    std::cout<<registered.message<<"\n";
    if(argc>2&&std::string(argv[2])=="--card") {
        if(argc<4)return 3;
        const hc::CatalogCard* c=cards.find(argv[3]);
        if(!c){std::cerr<<"Card not found\n";return 4;}
        printCard(*c);
        return 0;
    }
    if(argc>2&&std::string(argv[2])=="--starter") {
        if(argc<4)return 3;
        const int version=std::atoi(argv[3]);
        if(version!=1&&version!=2)return 3;
        const std::vector<const hc::CatalogCard*> pool=
            hc::StarterDeckBuilder::cardsInSet(cards,version);
        for(size_t i=0;i<pool.size();++i)printCard(*pool[i]);
        std::cout<<"Starter Ver. "<<version<<": "<<pool.size()<<" distinct set slots\n";
        return pool.size()==60?0:5;
    }
    if(argc>2&&std::string(argv[2])=="--demo") {
        hc::StarterDeck first,second;
        hc::Result a=hc::StarterDeckBuilder::build(cards,bridge,1,first);
        hc::Result b=hc::StarterDeckBuilder::build(cards,bridge,2,second);
        if(!a.ok||!b.ok) {
            std::cerr<<a.message<<" "<<b.message<<"\n";
            return 5;
        }
        hc::Result started=engine.start(first.cards,second.cards,
                                        first.starter,second.starter,0);
        if(!started.ok){std::cerr<<started.message<<"\n";return 6;}
        std::cout<<"STARTER VER. 1 vs STARTER VER. 2\n";
        for(int i=0;i<1000&&engine.phase()!=hc::Phase::Finished;++i) {
            const hc::AutoStep step=hc::AutoPlayer::step(engine);
            if(!step.result.ok){std::cerr<<step.result.message<<"\n";return 7;}
            if(step.phaseBefore==hc::Phase::Battle&&
               step.result.message=="Battle resolved")
                std::cout<<"Battle: "<<engine.lastPower(0)<<" vs "
                         <<engine.lastPower(1)<<"\n";
            if(step.phaseBefore==hc::Phase::Points)
                std::cout<<"Round "<<engine.round()<<": "
                         <<engine.getPlayer(0).points<<" vs "
                         <<engine.getPlayer(1).points<<"\n";
        }
        const bool finished=engine.phase()==hc::Phase::Finished;
        std::cout<<(finished?"Match finished":"Match step limit exceeded")<<"\n";
        return finished?0:8;
    }
    const char* sample[]={"St-1","St-2","St-49","St-62","St-111"};
    for(size_t i=0;i<sizeof(sample)/sizeof(sample[0]);++i) {
        const hc::CatalogCard* c=cards.find(sample[i]);
        if(c)printCard(*c);
    }
    std::cout<<"Commands: <csv> --card St-2 | --starter 1 | --starter 2 | --demo\n";
    return 0;
}
