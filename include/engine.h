#pragma once
#include <string>
#include <vector>
#include <map>
#include <random>
namespace hc {
enum class Kind { Digimon, Option };
enum class Level { III=3, IV=4, Perfect=5, Ultimate=6, Other=0 };
enum class Attack { A=0, B=1, C=2 };
enum class Phase { Setup, Preparation, Evolution, Battle, Points, Finished };
struct Card {
 int id; std::string name; Kind kind; Level level;
 Attack battleType; int power[3];
 int cancelAttack; int lostPoints[4];
 std::vector<int> evolvesFrom; int regularCost; int irregularCost;
 int optionAttackOverride; // -1 unless a verified A/B/C Plug-In
 bool requiresSpecialEvolution; bool hasUnimplementedEffect;
 Card() : id(0), kind(Kind::Digimon), level(Level::III),
   battleType(Attack::A), cancelAttack(-1), regularCost(0), irregularCost(0),
   optionAttackOverride(-1),
   requiresSpecialEvolution(false), hasUnimplementedEffect(false) {
     for(int i=0;i<3;++i)power[i]=0;
     lostPoints[0]=10;lostPoints[1]=20;lostPoints[2]=30;lostPoints[3]=40;
   }
};
struct Player {
 std::vector<int> deck, hand, discard, evolutionCost, options;
 int rookie, active, planned, points, gaugeCard;
 bool prepared, evolved;
 Player() : rookie(-1), active(-1), planned(-1), points(100),
            gaugeCard(-1), prepared(false), evolved(false) {}
};
struct Result {
 bool ok; std::string message;
 Result(bool success=false, const std::string& why=std::string()) : ok(success),message(why) {}
};
class Engine {
public:
 explicit Engine(unsigned seed=1);
 void registerCard(const Card& card);
 Result start(const std::vector<int>& a,const std::vector<int>& b,int starterA,int starterB,int firstPlayer);
 Result discardFromHand(int p,int cardId);
 Result replenish(int p);
 Result planEvolution(int p,int cardId);
 Result playBattleOption(int p,int cardId);
 Result commitPreparation(int p);
 Result evolve(int p,bool accept=true);
 Result resolveBattle();
 Result resolvePoints();
 const Player& getPlayer(int p) const { return players_[p]; }
 Phase phase() const { return phase_; }
 int firstPlayer() const { return first_; }
 int round() const { return round_; }
 int lastPower(int p) const { return lastPower_[p]; }
 int winner() const { return winner_; }
private:
 const Card* find(int id) const;
 bool valid(int p) const { return p>=0&&p<2; }
 bool remove(std::vector<int>& v,int id);
 bool validateDeck(const std::vector<int>& deck,int starter) const;
 void drawToSix(int p);
 void discardEvolved(int p);
 std::map<int,Card> cards_; Player players_[2]; Phase phase_;
 int first_,round_,winner_,lastPower_[2]; std::mt19937 rng_;
};
}
