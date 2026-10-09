#include "starter_decks.h"
#include "auto_player.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

int main(int argc,char** argv) {
    if(argc!=2)return 9;
    hc::CardCatalog catalog;
    assert(catalog.loadCSV(argv[1]).ok);
    assert(catalog.size()==111);
    assert(hc::StarterDeckBuilder::cardsInSet(catalog,1).size()==60);
    assert(hc::StarterDeckBuilder::cardsInSet(catalog,2).size()==60);
    assert(catalog.find("Bo-1")==0);
    assert(catalog.find("St-112")==0);
    const hc::CatalogCard* st1=catalog.find("St-1");
    const hc::CatalogCard* st2=catalog.find("St-2");
    const hc::CatalogCard* st62=catalog.find("St-62");
    assert(st1 && st2 && st62 && catalog.find("St-111"));
    assert(st1->power[0]==360 && st1->lost[0]==10);
    assert(st2->power[0]==440 && st2->lost[0]==20);
    assert(st2->evolutionRequirements=="Agumon:OO");
    assert(st2->isPlayableCore() && st62->isPlayableCore());
    assert(catalog.find("St-49")->kind=="Option");
    assert(!catalog.find("St-61")->isPlayableCore()); // unimplemented evolution

    hc::EngineCardBridge bridge(catalog);
    assert(bridge.numberToInternal("St-49")>0);
    assert(bridge.numberToInternal("St-50")>0);
    assert(bridge.numberToInternal("St-51")>0);
    assert(bridge.numberToInternal("St-101")==-1);
    hc::StarterDeck first,second;
    assert(hc::StarterDeckBuilder::build(catalog,bridge,1,first).ok);
    assert(hc::StarterDeckBuilder::build(catalog,bridge,2,second).ok);
    assert(first.cards.size()==30 && second.cards.size()==30);
    assert(first.starter==bridge.numberToInternal("St-1"));
    assert(second.starter==bridge.numberToInternal("St-62"));
    hc::Engine game(2026);
    assert(bridge.registerCoreCards(game).ok);
    assert(game.start(first.cards,second.cards,first.starter,second.starter,0).ok);
    int moves=0;
    while(game.phase()!=hc::Phase::Finished && moves<1000) {
        const hc::AutoStep step=hc::AutoPlayer::step(game);
        assert(step.result.ok);
        ++moves;
    }
    assert(game.phase()==hc::Phase::Finished);

    // Independent rules-level test: St-50 style C override during battle.
    bool checkedOverride=false;
    for(unsigned seed=1;seed<100&&!checkedOverride;++seed) {
        hc::Engine e(seed);
        std::vector<int> deck;
        for(int i=1;i<=9;++i) {
            hc::Card c;
            c.id=i;c.name="Rookie "+std::to_string(i);
            c.level=hc::Level::III;
            c.kind=hc::Kind::Digimon;
            c.battleType=hc::Attack::A;
            c.power[0]=100+i;
            c.power[1]=200+i;
            c.power[2]=300+i;
            e.registerCard(c);
            for(int copy=0;copy<3;++copy)deck.push_back(i);
        }
        hc::Card item;
        item.id=100;
        item.name="Defense Plug-In C";
        item.kind=hc::Kind::Option;
        item.optionAttackOverride=2;
        e.registerCard(item);
        for(int i=0;i<3;++i)deck.push_back(100);
        assert(e.start(deck,deck,1,2,0).ok);
        if(std::find(e.getPlayer(0).hand.begin(),e.getPlayer(0).hand.end(),100)
            ==e.getPlayer(0).hand.end())continue;
        assert(e.commitPreparation(0).ok);
        assert(e.commitPreparation(1).ok);
        assert(e.evolve(0).ok);
        assert(e.evolve(1).ok);
        assert(e.playBattleOption(0,100).ok);
        assert(e.getPlayer(0).options.size()==1);
        assert(e.resolveBattle().ok);
        assert(e.lastPower(0)==301);
        assert(e.resolvePoints().ok);
        assert(e.getPlayer(0).options.empty());
        checkedOverride=true;
    }
    assert(checkedOverride);
    std::cout<<"Starter deck and option tests PASS: 111 IDs; "
             <<moves<<" automatic actions\n";
    return 0;
}
