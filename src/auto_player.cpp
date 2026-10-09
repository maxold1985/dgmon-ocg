#include "auto_player.h"
namespace hc {
AutoStep AutoPlayer::step(Engine& engine) {
    const Phase phase=engine.phase();
    if(phase==Phase::Preparation) {
        const int first=engine.firstPlayer();
        const int player=engine.getPlayer(first).prepared?1-first:first;
        Result r=engine.replenish(player);
        if(!r.ok)return AutoStep(r,phase,player);
        const std::vector<int> hand=engine.getPlayer(player).hand;
        for(size_t i=0;i<hand.size();++i) {
            // Try each legal evolution; invalid attempts do not mutate Engine.
            if(engine.planEvolution(player,hand[i]).ok)break;
        }
        return AutoStep(engine.commitPreparation(player),phase,player);
    }
    if(phase==Phase::Evolution) {
        const int first=engine.firstPlayer();
        const int player=engine.getPlayer(first).evolved?1-first:first;
        return AutoStep(engine.evolve(player,true),phase,player);
    }
    if(phase==Phase::Battle)return AutoStep(engine.resolveBattle(),phase,-1);
    if(phase==Phase::Points)return AutoStep(engine.resolvePoints(),phase,-1);
    if(phase==Phase::Finished)return AutoStep(Result(false,"Game already finished"),phase,-1);
    return AutoStep(Result(false,"Game not started"),phase,-1);
}
}
