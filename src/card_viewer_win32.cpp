#ifdef _WIN32
#include "card_catalog.h"
#include "auto_player.h"
#include "starter_decks.h"
#include "card_viewer_paths.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace Gdiplus;

namespace {
const int W=1280;
const int H=840;
enum Action {
    NEW_GAME=1, REFILL, DISCARD, PLAN, PREPARE, EVOLVE, BATTLE, POINTS,
    PREVIOUS, NEXT, CHOOSE_HAND, CHOOSE_PREVIEW, AUTO_TOGGLE, SPEED
};
struct Hotspot {
    RectF rect;
    Action action;
    int index;
};
hc::CardCatalog catalog;
std::vector<const hc::CatalogCard*> collection;
std::unique_ptr<hc::EngineCardBridge> bridge;
hc::Engine engine(2026);
bool matchStarted=false;
bool autoRunning=false;
bool demoOnly=false;
unsigned autoInterval=1100;
const UINT_PTR AUTO_TIMER=401;
unsigned long demoTick=0;
size_t currentCard=0;
int selectedHand=-1;
std::wstring feedback=L"Pronto. Escolha NOVA PARTIDA ou examine as cartas.";
std::map<std::string,std::shared_ptr<Image> > textures;
std::deque<std::string> textureOrder;
std::set<std::string> missingImages;
std::vector<Hotspot> hotspots;
float screenScale=1.0f;
float screenX=0,screenY=0;

Color rgb(int r,int g,int b){return Color(255,(BYTE)r,(BYTE)g,(BYTE)b);}
std::wstring wide(const std::string& value) {
    if(value.empty())return L"";
    int len=MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,0,0);
    if(len<=0)return L"";
    std::vector<wchar_t> buffer((size_t)len);
    MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,&buffer[0],len);
    return std::wstring(&buffer[0]);
}
void fill(Graphics& g,REAL x,REAL y,REAL w,REAL h,Color color) {
    SolidBrush b(color);g.FillRectangle(&b,x,y,w,h);
}
void frame(Graphics& g,REAL x,REAL y,REAL w,REAL h,Color color,REAL penWidth=2) {
    Pen p(color,penWidth);g.DrawRectangle(&p,x,y,w,h);
}
void caption(Graphics& g,const std::wstring& text,REAL x,REAL y,int size,Color color,bool bold=false) {
    FontFamily family(L"Consolas");
    Font font(&family,(REAL)size,bold?FontStyleBold:FontStyleRegular,UnitPixel);
    SolidBrush brush(color);
    RectF bounds(x,y,(REAL)W-x-8,45);
    g.DrawString(text.c_str(),-1,&font,bounds,0,&brush);
}
void panel(Graphics& g,int x,int y,int w,int h,const std::wstring& name,Color c) {
    fill(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,rgb(18,42,59));
    frame(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,c,2);
    fill(g,(REAL)x,(REAL)y,(REAL)w,25,c);
    caption(g,name,(REAL)x+8,(REAL)y+3,15,rgb(7,22,34),true);
}
void registerHit(Action a,int index,REAL x,REAL y,REAL w,REAL h) {
    Hotspot hit;hit.rect=RectF(x,y,w,h);hit.action=a;hit.index=index;hotspots.push_back(hit);
}
void button(Graphics& g,Action a,const std::wstring& label,int x,int y,int w,int h,bool available) {
    Color edge=available?rgb(94,213,220):rgb(76,96,107);
    fill(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,available?rgb(25,74,88):rgb(34,45,52));
    frame(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,edge);
    caption(g,label,(REAL)x+10,(REAL)y+8,15,available?rgb(237,247,247):rgb(130,145,150),true);
    if(available)registerHit(a,0,(REAL)x,(REAL)y,(REAL)w,(REAL)h);
}
bool fileExists(const std::wstring& path) {
    DWORD attr=GetFileAttributesW(path.c_str());
    return attr!=INVALID_FILE_ATTRIBUTES && !(attr&FILE_ATTRIBUTE_DIRECTORY);
}
std::shared_ptr<Image> imageFor(const std::string& id) {
    std::map<std::string,std::shared_ptr<Image> >::iterator found=textures.find(id);
    if(found!=textures.end())return found->second;
    if(missingImages.count(id))return std::shared_ptr<Image>();
    const std::string base=std::string(HC_SOURCE_ROOT)+"/data/";
    const char* dirs[]={"card_images_original/","card_images/"};
    const char* exts[]={".jpg",".jpeg",".png"};
    for(size_t d=0;d<2;++d)for(size_t e=0;e<3;++e) {
        std::wstring path=wide(base+dirs[d]+id+exts[e]);
        if(!fileExists(path))continue;
        std::shared_ptr<Image> img(Image::FromFile(path.c_str(),FALSE));
        if(img && img->GetLastStatus()==Ok && img->GetWidth() && img->GetHeight()) {
            textures[id]=img;
            textureOrder.push_back(id);
            while(textureOrder.size()>20) {
                textures.erase(textureOrder.front());
                textureOrder.pop_front();
            }
            return img;
        }
    }
    missingImages.insert(id);
    return std::shared_ptr<Image>();
}
const hc::CatalogCard* cardFromInternal(int number) {
    if(!bridge||number<0)return 0;
    return catalog.find(bridge->internalToNumber(number));
}
const hc::CatalogCard* focusCard() {
    if(matchStarted&&selectedHand>=0) {
        const std::vector<int>& hand=engine.getPlayer(0).hand;
        if((size_t)selectedHand<hand.size())return cardFromInternal(hand[(size_t)selectedHand]);
    }
    if(matchStarted&&autoRunning) {
        const hc::CatalogCard* active=cardFromInternal(engine.getPlayer(0).active);
        if(active)return active;
    }
    return collection.empty()?0:collection[currentCard];
}
void cardBox(Graphics& g,const hc::CatalogCard* c,REAL x,REAL y,REAL w,REAL h,bool highlight=false) {
    fill(g,x,y,w,h,rgb(8,25,41));
    frame(g,x,y,w,h,highlight?rgb(255,223,103):rgb(96,158,177),highlight?3:2);
    if(!c) {
        caption(g,L"VAZIO",x+10,y+h/2-10,14,rgb(131,161,179));
        return;
    }
    std::shared_ptr<Image> img=imageFor(c->id);
    if(img) {
        REAL iw=(REAL)img->GetWidth(),ih=(REAL)img->GetHeight();
        REAL scale=std::min((w-8)/iw,(h-8)/ih);
        REAL rw=iw*scale,rh=ih*scale;
        g.DrawImage(img.get(),RectF(x+(w-rw)/2,y+(h-rh)/2,rw,rh));
    } else {
        fill(g,x+5,y+5,w-10,h-10,rgb(38,66,93));
        caption(g,wide(c->id),x+9,y+h/2-14,w>90?18:12,rgb(249,224,148),true);
        if(w>90)caption(g,L"SEM JPG",x+9,y+h/2+14,12,rgb(229,164,148));
    }
}
void cardBack(Graphics& g,int x,int y,int w,int h) {
    fill(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,rgb(24,81,111));
    frame(g,(REAL)x,(REAL)y,(REAL)w,(REAL)h,rgb(140,219,216),2);
    frame(g,(REAL)x+6,(REAL)y+6,(REAL)w-12,(REAL)h-12,rgb(76,157,183),1);
    caption(g,L"D",x+w/2-8,y+h/2-15,w>60?30:17,rgb(230,207,111),true);
}
const wchar_t* phaseText(hc::Phase p) {
    switch(p) {
        case hc::Phase::Setup:return L"SETUP";
        case hc::Phase::Preparation:return L"PREPARACAO";
        case hc::Phase::Evolution:return L"EVOLUCAO";
        case hc::Phase::Battle:return L"BATALHA";
        case hc::Phase::Points:return L"PONTOS";
        case hc::Phase::Finished:return L"FIM";
    }
    return L"?";
}
bool canAct(hc::Phase phase) {return matchStarted&&engine.phase()==phase;}
bool canPrepare() {return canAct(hc::Phase::Preparation)&&!engine.getPlayer(0).prepared;}
void drawZones(Graphics& g) {
    panel(g,16,67,202,160,L"PONTOS CPU - ST2",rgb(210,120,103));
    panel(g,16,239,202,105,L"REQUISITOS",rgb(220,140,169));
    panel(g,16,356,202,115,L"EVOLUCAO",rgb(223,160,88));
    panel(g,16,483,202,140,L"PONTOS VOCE - ST1",rgb(94,199,149));
    panel(g,16,635,202,110,L"CUSTO DE EVO",rgb(220,140,169));
    if(matchStarted) {
        const hc::Player& cpu=engine.getPlayer(1);
        const hc::Player& me=engine.getPlayer(0);
        caption(g,std::to_wstring(cpu.points)+L" / 100",33,115,29,rgb(253,229,196),true);
        caption(g,L"NET "+std::to_wstring(cpu.deck.size())+L"  DARK "+std::to_wstring(cpu.discard.size()),29,174,15,rgb(232,237,243));
        caption(g,std::to_wstring(me.points)+L" / 100",33,531,29,rgb(191,244,214),true);
        caption(g,L"NET "+std::to_wstring(me.deck.size())+L"  DARK "+std::to_wstring(me.discard.size()),29,584,15,rgb(232,237,243));
        caption(g,L"Reservadas: "+std::to_wstring(me.evolutionCost.size()),28,682,15,rgb(235,233,244));
        const hc::CatalogCard* planned=cardFromInternal(me.planned);
        if(planned)caption(g,wide(planned->id),27,400,24,rgb(254,236,196),true);
        else caption(g,L"Nenhuma",27,408,15,rgb(230,237,247));
    } else {
        caption(g,L"100 / 100",33,115,28,rgb(232,233,239),true);
        caption(g,L"100 / 100",33,531,28,rgb(232,233,239),true);
        caption(g,L"Sem partida",29,280,16,rgb(218,225,230));
        caption(g,L"Sem partida",29,410,16,rgb(218,225,230));
        caption(g,L"Sem partida",29,678,16,rgb(218,225,230));
    }
}
void drawField(Graphics& g) {
    panel(g,234,66,687,264,L"STARTER VER. 2 - CPU",rgb(187,113,105));
    panel(g,234,344,687,398,L"STARTER VER. 1 - JOGADOR",rgb(75,178,171));
    // Opponent hand: cards are always face down.
    caption(g,L"MAO",249,103,13,rgb(211,228,229),true);
    int enemyCards=matchStarted?(int)engine.getPlayer(1).hand.size():6;
    if(enemyCards>6)enemyCards=6;
    for(int i=0;i<enemyCards;++i)cardBack(g,250+i*52,130,45,65);
    caption(g,L"DIGIMON",481,98,15,rgb(247,227,190),true);
    cardBox(g,matchStarted?cardFromInternal(engine.getPlayer(1).active):(collection.size()>1?collection[(currentCard+7)%collection.size()]:0),482,119,122,174);
    caption(g,L"SUPORTE",627,121,14,rgb(216,231,231));
    cardBox(g,0,625,149,94,125);
    caption(g,L"NET OCEAN",782,110,14,rgb(216,231,231));
    cardBack(g,780,141,87,126);
    caption(g,matchStarted?std::to_wstring(engine.getPlayer(1).deck.size()):L"0",802,276,16,rgb(251,225,159),true);
    caption(g,L"DARK: "+std::to_wstring(matchStarted?engine.getPlayer(1).discard.size():0),771,302,13,rgb(234,211,189));
    fill(g,240,331,675,12,rgb(236,187,95));
    caption(g,L"BATTLE   <  VS  >",480,330,14,rgb(20,29,42),true);
    caption(g,L"DIGIMON",481,372,15,rgb(201,239,234),true);
    cardBox(g,matchStarted?cardFromInternal(engine.getPlayer(0).active):(collection.empty()?0:collection[currentCard]),482,397,122,174);
    caption(g,L"SUPORTE",626,388,14,rgb(216,231,231));
    cardBox(g,0,625,412,94,131);
    caption(g,L"NET OCEAN",782,388,14,rgb(216,231,231));
    cardBack(g,779,415,87,123);
    caption(g,matchStarted?std::to_wstring(engine.getPlayer(0).deck.size()):L"0",802,547,16,rgb(251,225,159),true);
    caption(g,L"DARK: "+std::to_wstring(matchStarted?engine.getPlayer(0).discard.size():0),770,571,13,rgb(216,236,233));
    caption(g,L"SUA MAO - clique uma carta",256,591,15,rgb(255,230,174),true);
    const std::vector<int>* hand=matchStarted?&engine.getPlayer(0).hand:0;
    if(hand) {
        for(size_t i=0;i<hand->size()&&i<6;++i) {
            REAL x=253+(REAL)i*106;
            const hc::CatalogCard* c=cardFromInternal((*hand)[i]);
            cardBox(g,c,x,614,93,115,selectedHand==(int)i);
            registerHit(CHOOSE_HAND,(int)i,x,614,93,115);
        }
    } else if(!collection.empty()) {
        for(size_t i=0;i<6 && i<collection.size();++i) {
            const size_t index=(currentCard+i)%collection.size();
            REAL x=253+(REAL)i*106;
            cardBox(g,collection[index],x,614,93,115,i==0);
            registerHit(CHOOSE_PREVIEW,(int)index,x,614,93,115);
        }
        caption(g,L"MODO PREVIA - cartas do catalogo (sem partida)",252,732,12,rgb(252,201,120));
    }
}
void drawSidebar(Graphics& g) {
    panel(g,938,66,326,688,L"CARTA / COMANDOS",rgb(221,181,102));
    const hc::CatalogCard* c=focusCard();
    cardBox(g,c,998,101,206,294);
    if(c) {
        caption(g,wide(c->id)+L"  "+wide(c->kind),953,399,17,rgb(255,222,154),true);
        caption(g,wide(c->name),953,426,16,rgb(223,244,245));
        caption(g,L"LEVEL "+wide(c->level)+L" | TYPE "+wide(c->battleType),953,451,13,rgb(204,225,238));
        if(c->kind=="Digimon")
            caption(g,L"A "+std::to_wstring(c->power[0])+L"   B "+
                    std::to_wstring(c->power[1])+L"   C "+
                    std::to_wstring(c->power[2]),953,473,14,rgb(247,220,155));
        else caption(g,L"OPTION: "+wide(c->effectStatus),953,474,12,rgb(247,220,155));
        const bool implemented=c->isPlayableCore()||
           c->id=="St-49"||c->id=="St-50"||c->id=="St-51";
        caption(g,implemented?L"PROGRAMADA | botao direito: detalhes":
            L"EFEITO PENDENTE | botao direito: detalhes",953,492,11,
            implemented?rgb(115,214,155):rgb(255,157,132));
    }
    button(g,NEW_GAME,L"NOVA",949,509,91,40,true);
    button(g,AUTO_TOGGLE,autoRunning?L"PAUSAR":L"AUTO JOGAR",1047,509,136,40,true);
    button(g,SPEED,autoInterval==550?L"0.5s":(autoInterval==1100?L"1.1s":L"2.2s"),1190,509,64,40,true);
    button(g,REFILL,L"REPOR",949,561,146,36,canPrepare());
    button(g,DISCARD,L"DESCARTAR",1107,561,147,36,canPrepare()&&selectedHand>=0);
    button(g,PLAN,L"PLANEJAR EVO",949,603,146,36,canPrepare()&&selectedHand>=0);
    button(g,PREPARE,L"PREPARAR",1107,603,147,36,canPrepare());
    button(g,EVOLVE,L"EVOLUIR",949,645,146,36,canAct(hc::Phase::Evolution));
    bool optionSelected=false;
    if(matchStarted&&selectedHand>=0 &&
       (size_t)selectedHand<engine.getPlayer(0).hand.size()) {
        const hc::CatalogCard* chosen=cardFromInternal(
            engine.getPlayer(0).hand[(size_t)selectedHand]);
        optionSelected=chosen&&chosen->kind=="Option";
    }
    button(g,BATTLE,optionSelected?L"USAR ITEM":L"BATALHA",
           1107,645,147,36,canAct(hc::Phase::Battle));
    button(g,POINTS,L"PONTOS",949,687,146,36,canAct(hc::Phase::Points));
    button(g,PREVIOUS,L"< ANTERIOR",1107,687,70,36,!collection.empty());
    button(g,NEXT,L">",1183,687,71,36,!collection.empty());
    caption(g,L"A: auto/pausa | N: nova | +/-: velocidade",949,728,12,rgb(211,226,233));
}
void render(Graphics& g) {
    hotspots.clear();
    fill(g,0,0,W,H,rgb(9,30,43));
    Pen grid(rgb(15,59,69),1);
    for(int x=0;x<W;x+=28)g.DrawLine(&grid,x,0,x,H);
    for(int y=0;y<H;y+=28)g.DrawLine(&grid,0,y,W,y);
    fill(g,0,0,W,55,rgb(14,70,86));
    caption(g,L"DIGIMON CARD GAME  |  HYPER COLOSSEUM",20,10,25,rgb(254,224,141),true);
    std::wstring mode=demoOnly?L"AUTO DEMO":(autoRunning?L"AUTO JOGO":(matchStarted?L"MANUAL":L"PREVIA"));
    caption(g,L"STARTER ST1 x ST2 | "+mode+L" | "+std::wstring(phaseText(matchStarted?engine.phase():hc::Phase::Setup)),662,18,16,rgb(224,248,247),true);
    drawZones(g);
    drawField(g);
    drawSidebar(g);
    fill(g,0,766,W,74,rgb(10,61,78));
    caption(g,L"STATUS: "+feedback,21,778,16,rgb(245,240,199));
    caption(g,L"Auto: motor de 2 jogadores com cartas verificadas. Sem regras: demonstracao visual. ESC fecha",21,809,12,rgb(195,217,229));
}
bool beginMatch() {
    if(!bridge)return false;
    matchStarted=false;
    selectedHand=-1;
    hc::StarterDeck first,second;
    hc::Result a=hc::StarterDeckBuilder::build(catalog,*bridge,1,first);
    if(!a.ok){feedback=wide(a.message);return false;}
    hc::Result b=hc::StarterDeckBuilder::build(catalog,*bridge,2,second);
    if(!b.ok){feedback=wide(b.message);return false;}
    hc::Engine fresh(2026);
    hc::Result registered=bridge->registerCoreCards(fresh);
    if(!registered.ok){feedback=wide(registered.message);return false;}
    hc::Result started=fresh.start(first.cards,second.cards,first.starter,second.starter,0);
    if(!started.ok){feedback=wide(started.message);return false;}
    engine=fresh;
    matchStarted=true;
    selectedHand=-1;
    feedback=L"Starter Ver. 1 (voce) x Starter Ver. 2 (CPU) | 30 cartas cada.";
    return true;
}

void advanceAutomatic() {
    if(!autoRunning)return;
    if(demoOnly) {
        if(collection.empty()) {
            autoRunning=false;
            return;
        }
        currentCard=(currentCard+1)%collection.size();
        ++demoTick;
        feedback=L"DEMO VISUAL "+std::to_wstring(demoTick)+L": "+
                 wide(collection[currentCard]->id)+
                 L" | sem regras suficientes para simular batalhas.";
        return;
    }
    if(!matchStarted) {
        autoRunning=false;
        feedback=L"Auto interrompido: partida nao iniciada.";
        return;
    }
    hc::AutoStep step=hc::AutoPlayer::step(engine);
    if(!step.result.ok) {
        autoRunning=false;
        feedback=L"Auto interrompido: "+wide(step.result.message);
        return;
    }
    selectedHand=-1;
    std::wstring label;
    switch(step.phaseBefore) {
        case hc::Phase::Preparation:label=L"PREPARACAO";break;
        case hc::Phase::Evolution:label=L"EVOLUCAO";break;
        case hc::Phase::Battle:
            if(step.result.message=="Battle option activated")
                label=L"PLUG-IN "+std::wstring(step.actor==0?L"ST1":L"ST2");
            else label=L"BATALHA "+std::to_wstring(engine.lastPower(0))+
                L" x "+std::to_wstring(engine.lastPower(1));
            break;
        case hc::Phase::Points:
            label=L"PONTOS "+std::to_wstring(engine.getPlayer(0).points)+
                L" x "+std::to_wstring(engine.getPlayer(1).points);break;
        default:label=L"FASE";break;
    }
    feedback=L"RODADA "+std::to_wstring(engine.round())+L" | "+label;
    if(step.actor>=0)feedback+=(step.actor==0?L" | VOCE":L" | CPU");
    if(engine.phase()==hc::Phase::Finished) {
        autoRunning=false;
        feedback=L"FIM! "+std::to_wstring(engine.getPlayer(0).points)+
                 L" x "+std::to_wstring(engine.getPlayer(1).points);
    }
}

void gameAction(Action a,int index) {
    if(a==PREVIOUS || a==NEXT) {
        if(collection.empty())return;
        currentCard=(currentCard+collection.size()+(a==NEXT?1:collection.size()-1))%collection.size();
        selectedHand=-1;
        return;
    }
    if(a==CHOOSE_PREVIEW) {
        if(index>=0 && (size_t)index<collection.size())currentCard=(size_t)index;
        selectedHand=-1;
        feedback=L"Visualizacao de carta Starter. Pressione N para iniciar a partida.";
        return;
    }
    if(a==CHOOSE_HAND) {
        selectedHand=index;
        feedback=L"Carta selecionada. Use DESCARTAR ou PLANEJAR EVO.";
        return;
    }
    if(a==AUTO_TOGGLE) {
        if(autoRunning) {
            autoRunning=false;
            feedback=L"Jogo automatico pausado. Pressione AUTO para continuar.";
        } else {
            if(!matchStarted && !demoOnly) {
                if(!beginMatch()) {
                    demoOnly=true;
                    demoTick=0;
                    feedback=L"DEMO VISUAL: faltam cartas verificadas para uma partida oficial.";
                }
            } else if(matchStarted && engine.phase()==hc::Phase::Finished) {
                if(!beginMatch()) {
                    demoOnly=true;
                    demoTick=0;
                }
            }
            autoRunning=true;
            if(!demoOnly)feedback=L"Jogo automatico ativo: o motor executa um passo por vez.";
        }
        return;
    }
    if(a==SPEED) {
        autoInterval=autoInterval==550?1100:(autoInterval==1100?2200:550);
        feedback=L"Velocidade: passo a cada "+std::to_wstring(autoInterval)+L" ms";
        return;
    }
    if(a==NEW_GAME) {
        autoRunning=false;
        demoOnly=false;
        beginMatch();
        return;
    }
    if(!matchStarted){feedback=L"Sem partida validada. Use AUTO para demonstracao visual.";return;}
    if(autoRunning) {
        feedback=L"Pause o AUTO para jogar manualmente.";
        return;
    }
    hc::Result r;
    switch(a) {
        case REFILL:r=engine.replenish(0);break;
        case DISCARD: {
            const std::vector<int>& hand=engine.getPlayer(0).hand;
            if(selectedHand<0||(size_t)selectedHand>=hand.size()){feedback=L"Selecione uma carta da mao.";return;}
            r=engine.discardFromHand(0,hand[(size_t)selectedHand]);selectedHand=-1;break;
        }
        case PLAN: {
            const std::vector<int>& hand=engine.getPlayer(0).hand;
            if(selectedHand<0||(size_t)selectedHand>=hand.size()){feedback=L"Selecione uma carta da mao.";return;}
            r=engine.planEvolution(0,hand[(size_t)selectedHand]);
            if(r.ok)selectedHand=-1;
            break;
        }
        case PREPARE: {
            const int first=engine.firstPlayer();
            if(first==1){r=engine.commitPreparation(1);if(!r.ok)break;}
            r=engine.commitPreparation(0);
            if(r.ok && first==0)r=engine.commitPreparation(1);
            break;
        }
        case EVOLVE: {
            const int first=engine.firstPlayer();
            if(first==1){r=engine.evolve(1);if(!r.ok)break;}
            r=engine.evolve(0);
            if(r.ok && first==0)r=engine.evolve(1);
            break;
        }
        case BATTLE: {
            const std::vector<int>& hand=engine.getPlayer(0).hand;
            const hc::CatalogCard* chosen=
                selectedHand>=0&&(size_t)selectedHand<hand.size()?
                cardFromInternal(hand[(size_t)selectedHand]):0;
            if(chosen&&chosen->kind=="Option") {
                r=engine.playBattleOption(0,hand[(size_t)selectedHand]);
                if(r.ok)selectedHand=-1;
            } else r=engine.resolveBattle();
            break;
        }
        case POINTS:r=engine.resolvePoints();selectedHand=-1;break;
        default:return;
    }
    feedback=wide(r.message);
    if(!r.ok)feedback=L"ACAO NEGADA: "+feedback;
    else if(a==BATTLE && r.message=="Battle resolved") {
        feedback+=L"  VOCE "+std::to_wstring(engine.lastPower(0))+
                  L" x "+std::to_wstring(engine.lastPower(1))+L" CPU";
    }
    else if(a==POINTS && engine.phase()==hc::Phase::Finished)
        feedback=L"FIM DA PARTIDA. Clique NOVA PARTIDA.";
}
int cardNumber(const hc::CatalogCard* c) {
    return std::atoi(c->id.c_str()+3);
}
LRESULT CALLBACK windowProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
        case WM_ERASEBKGND:return 1;
        case WM_TIMER:
            if(wp==AUTO_TIMER && autoRunning) {
                static DWORD lastStep=0;
                DWORD now=GetTickCount();
                if(now-lastStep>=autoInterval) {
                    lastStep=now;
                    advanceAutomatic();
                    InvalidateRect(hwnd,0,FALSE);
                }
                return 0;
            }
            break;
        case WM_SIZE:InvalidateRect(hwnd,0,FALSE);return 0;
        case WM_LBUTTONDOWN: {
            if(screenScale<=0)return 0;
            REAL px=((REAL)GET_X_LPARAM(lp)-screenX)/screenScale;
            REAL py=((REAL)GET_Y_LPARAM(lp)-screenY)/screenScale;
            for(std::vector<Hotspot>::reverse_iterator i=hotspots.rbegin();i!=hotspots.rend();++i) {
                if(i->rect.Contains(px,py)) {
                    gameAction(i->action,i->index);
                    InvalidateRect(hwnd,0,FALSE);
                    break;
                }
            }
            return 0;
        }
        case WM_KEYDOWN:
            if(wp==VK_ESCAPE){DestroyWindow(hwnd);return 0;}
            if(wp==VK_LEFT)gameAction(PREVIOUS,0);
            else if(wp==VK_RIGHT)gameAction(NEXT,0);
            else if(wp=='N')gameAction(NEW_GAME,0);
            else if(wp=='R')gameAction(REFILL,0);
            else if(wp=='A' || wp==VK_SPACE)gameAction(AUTO_TOGGLE,0);
            else if(wp==VK_ADD || wp==VK_OEM_PLUS || wp==VK_SUBTRACT || wp==VK_OEM_MINUS)gameAction(SPEED,0);
            else if(wp==VK_RETURN)gameAction(PREPARE,0);
            else break;
            InvalidateRect(hwnd,0,FALSE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc=BeginPaint(hwnd,&ps);
            RECT r;GetClientRect(hwnd,&r);
            const REAL cw=(REAL)(r.right-r.left);
            const REAL ch=(REAL)(r.bottom-r.top);
            screenScale=std::min(cw/W,ch/H);
            screenX=(cw-W*screenScale)*0.5f;
            screenY=(ch-H*screenScale)*0.5f;
            Bitmap canvas(W,H,PixelFormat32bppARGB);
            Graphics offscreen(&canvas);
            offscreen.SetSmoothingMode(SmoothingModeAntiAlias);
            offscreen.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            render(offscreen);
            Graphics screen(hdc);
            screen.Clear(rgb(4,14,20));
            screen.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            screen.DrawImage(&canvas,screenX,screenY,W*screenScale,H*screenScale);
            EndPaint(hwnd,&ps);
            return 0;
        }
        case WM_DESTROY:
            KillTimer(hwnd,AUTO_TIMER);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
} // namespace

int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR,int show) {
    GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken=0;
    if(GdiplusStartup(&gdiplusToken,&gdiplusInput,0)!=Ok)return 1;
    hc::Result load=catalog.loadCSV(HC_CARDS_CSV);
    if(!load.ok) {
        MessageBoxW(0,wide(load.message).c_str(),L"Falha ao carregar catalogo",MB_OK|MB_ICONERROR);
        GdiplusShutdown(gdiplusToken);
        return 2;
    }
    collection=catalog.all();
    std::sort(collection.begin(),collection.end(),
        [](const hc::CatalogCard* a,const hc::CatalogCard* b){return cardNumber(a)<cardNumber(b);});
    bridge.reset(new hc::EngineCardBridge(catalog));
    WNDCLASSW wc={};
    wc.lpfnWndProc=windowProc;
    wc.hInstance=inst;
    wc.hCursor=LoadCursor(0,IDC_ARROW);
    wc.lpszClassName=L"HyperColosseumBoardWindow";
    if(!RegisterClassW(&wc)){GdiplusShutdown(gdiplusToken);return 3;}
    HWND hwnd=CreateWindowW(wc.lpszClassName,L"Digimon Hyper Colosseum | Tabuleiro",
        WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1320,900,0,0,inst,0);
    if(!hwnd){GdiplusShutdown(gdiplusToken);return 4;}
    ShowWindow(hwnd,show);
    UpdateWindow(hwnd);
    gameAction(AUTO_TOGGLE,0);
    if(!SetTimer(hwnd,AUTO_TIMER,250,0)) {
        autoRunning=false;
        feedback=L"Nao foi possivel inicializar o temporizador.";
    }
    InvalidateRect(hwnd,0,FALSE);
    MSG message;
    while(GetMessageW(&message,0,0,0)>0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    textures.clear();
    GdiplusShutdown(gdiplusToken);
    return (int)message.wParam;
}
#endif
