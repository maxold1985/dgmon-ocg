#include "auto_player.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    hc::Engine game(9001);
    std::vector<int> deck;
    for(int i=1;i<=10;++i) {
        hc::Card card;
        card.id=i;
        card.name="Test Digimon "+std::to_string(i);
        card.kind=hc::Kind::Digimon;
        card.level=hc::Level::III;
        card.battleType=hc::Attack::A;
        card.power[0]=100*i;
        card.power[1]=100*i;
        card.power[2]=100*i;
        game.registerCard(card);
        for(int copy=0;copy<3;++copy)deck.push_back(i);
    }
    const hc::Result started=game.start(deck,deck,1,2,0);
    assert(started.ok);
    int steps=0;
    while(game.phase()!=hc::Phase::Finished && steps<1000) {
        const hc::AutoStep step=hc::AutoPlayer::step(game);
        assert(step.result.ok);
        ++steps;
    }
    assert(game.phase()==hc::Phase::Finished);
    assert(steps>10);
    assert(game.getPlayer(0).points<=0);
    assert(game.getPlayer(1).points==100);
    const hc::AutoStep complete=hc::AutoPlayer::step(game);
    assert(!complete.result.ok);
    std::cout<<"Automatic duel tests PASS: "<<steps<<" steps\n";
    return 0;
}
