#include "engine.h"
#include <algorithm>
namespace hc {
Engine::Engine(unsigned seed):phase_(Phase::Setup),first_(0),round_(0),winner_(-1),rng_(seed){lastPower_[0]=lastPower_[1]=0;}
void Engine::registerCard(const Card& c){cards_[c.id]=c;}
const Card* Engine::find(int id) const {auto it=cards_.find(id);return it==cards_.end()?nullptr:&it->second;}
bool Engine::remove(std::vector<int>& v,int id){auto it=std::find(v.begin(),v.end(),id);if(it==v.end())return false;v.erase(it);return true;}
bool Engine::validateDeck(const std::vector<int>& d,int starter) const {
 if(d.size()!=30)return false; std::map<std::string,int> names; bool found=false;
 for(int id:d){const Card* c=find(id);if(!c)return false;if(++names[c->name]>3)return false;if(id==starter&&c->kind==Kind::Digimon&&c->level==Level::III)found=true;}
 return found;
}
void Engine::drawToSix(int p){Player& s=players_[p];while(s.hand.size()<6&&!s.deck.empty()){s.hand.push_back(s.deck.back());s.deck.pop_back();}}
Result Engine::start(const std::vector<int>& a,const std::vector<int>& b,int sa,int sb,int first){
 if(phase_!=Phase::Setup)return {false,"Already started"};
 if(first<0||first>1||!validateDeck(a,sa)||!validateDeck(b,sb))return {false,"Invalid 30-card deck or Level III starter"};
 const std::vector<int>* decks[2]={&a,&b};int starters[2]={sa,sb};
 for(int p=0;p<2;++p){Player& s=players_[p];s=Player();s.deck=*decks[p];remove(s.deck,starters[p]);s.rookie=s.active=starters[p];std::shuffle(s.deck.begin(),s.deck.end(),rng_);s.gaugeCard=s.deck.back();s.deck.pop_back();drawToSix(p);}
 first_=first;round_=1;phase_=Phase::Preparation;return {true,"Hyper Colosseum setup complete"};
}
Result Engine::discardFromHand(int p,int id){if(!valid(p)||phase_!=Phase::Preparation||players_[p].prepared)return {false,"Wrong phase"};Player& s=players_[p];if(!remove(s.hand,id))return {false,"Not in hand"};s.discard.push_back(id);return {true,"Discarded"};}
Result Engine::replenish(int p){if(!valid(p)||phase_!=Phase::Preparation||players_[p].prepared)return {false,"Wrong phase"};drawToSix(p);return {true,"Hand replenished"};}
Result Engine::planEvolution(int p,int id){
 if(!valid(p)||phase_!=Phase::Preparation||players_[p].prepared)return {false,"Wrong phase"};Player& s=players_[p];const Card* next=find(id);const Card* prev=find(s.active);
 if(!next||!prev||next->kind!=Kind::Digimon||next->requiresSpecialEvolution||next->hasUnimplementedEffect||!s.evolutionCost.empty()||s.planned!=-1)return {false,"Unsupported or invalid evolution"};
 if(std::find(s.hand.begin(),s.hand.end(),id)==s.hand.end()||std::find(next->evolvesFrom.begin(),next->evolvesFrom.end(),prev->id)==next->evolvesFrom.end())return {false,"Evolution requirement not met"};
 if((int)next->level<=(int)prev->level)return {false,"Evolution level invalid"};
 int cost=next->regularCost+next->irregularCost;
 if(cost<0||s.deck.size()<(size_t)cost)return {false,"Insufficient Net Ocean"};
 remove(s.hand,id);s.planned=id;
 for(int i=0;i<cost;++i){s.evolutionCost.push_back(s.deck.back());s.deck.pop_back();}
 return {true,"Evolution prepared; costs reserved face down"};
}
Result Engine::commitPreparation(int p){
 if(!valid(p)||phase_!=Phase::Preparation||p!=(players_[first_].prepared?1-first_:first_)||players_[p].prepared)return {false,"Wrong preparation order"};
 if(players_[p].hand.size()>6)return {false,"Hand exceeds six cards"};
 players_[p].prepared=true;if(players_[0].prepared&&players_[1].prepared)phase_=Phase::Evolution;
 return {true,"Preparation complete"};
}
Result Engine::evolve(int p,bool accept){
 if(!valid(p)||phase_!=Phase::Evolution||p!=(players_[first_].evolved?1-first_:first_)||players_[p].evolved)return {false,"Wrong evolution order"};
 Player& s=players_[p];if(s.planned!=-1){if(accept){if(s.active!=s.rookie)s.discard.push_back(s.active);s.active=s.planned;}else s.discard.push_back(s.planned);s.planned=-1;}
 s.discard.insert(s.discard.end(),s.evolutionCost.begin(),s.evolutionCost.end());s.evolutionCost.clear();s.evolved=true;
 if(players_[0].evolved&&players_[1].evolved)phase_=Phase::Battle;
 return {true,"Evolution phase action complete"};
}
Result Engine::resolveBattle(){
 if(phase_!=Phase::Battle)return {false,"Wrong phase"};const Card* c[2]={find(players_[0].active),find(players_[1].active)};
 if(!c[0]||!c[1]||c[0]->hasUnimplementedEffect||c[1]->hasUnimplementedEffect)return {false,"Missing card or unimplemented card effect"};
 for(int p=0;p<2;++p){int attack=(int)c[1-p]->battleType;lastPower_[p]=c[p]->power[attack];}
 for(int p=0;p<2;++p){int attack=(int)c[1-p]->battleType;if(attack==2&&c[p]->cancelAttack==(int)c[p]->battleType)lastPower_[1-p]=0;}
 winner_=lastPower_[0]==lastPower_[1]?-1:(lastPower_[0]>lastPower_[1]?0:1);phase_=Phase::Points;
 return {true,"Battle resolved"};
}
void Engine::discardEvolved(int p){Player& s=players_[p];if(s.active!=s.rookie){s.discard.push_back(s.active);s.active=s.rookie;}}
Result Engine::resolvePoints(){
 if(phase_!=Phase::Points)return {false,"Wrong phase"};
 if(winner_==-1){players_[0].points-=10;players_[1].points-=10;}
 else {int loser=1-winner_;const Card* losing=find(players_[loser].active);const Card* winning=find(players_[winner_].active);int ix=(int)winning->level-3;if(ix<0||ix>3)return {false,"Unsupported winning level"};players_[loser].points-=losing->lostPoints[ix];discardEvolved(loser);first_=winner_;}
 for(int p=0;p<2;++p){Player& s=players_[p];s.discard.insert(s.discard.end(),s.options.begin(),s.options.end());s.options.clear();if(s.deck.empty()){discardEvolved(p);std::shuffle(s.discard.begin(),s.discard.end(),rng_);s.deck.swap(s.discard);}s.prepared=false;s.evolved=false;}
 if(players_[0].points<=0||players_[1].points<=0){phase_=Phase::Finished;return {true,"Game finished"};}
 ++round_;phase_=Phase::Preparation;return {true,"Points calculated; next round"};
}
}
