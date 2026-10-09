#ifdef _WIN32
#include "card_catalog.h"
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
static hc::CardCatalog catalog;
static std::vector<const hc::CatalogCard*> cards;
static size_t selected = 0;
static std::wstring imageDir = L"data\\card_images_original\\";
static std::wstring widen(const std::string& s) {
    if(s.empty())return L"";
    int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,0,0);
    if(n<=0)return L"";
    std::wstring w(n,L'\0');
    MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,&w[0],n);
    w.resize(n-1);
    return w;
}
static void label(Graphics& g,const std::wstring& s,int x,int y,int size,Color color) {
    FontFamily family(L"Segoe UI");
    Font font(&family,(REAL)size,FontStyleRegular,UnitPixel);
    SolidBrush brush(color);
    g.DrawString(s.c_str(),-1,&font,PointF((REAL)x,(REAL)y),&brush);
}
static void draw(HDC dc,RECT rc) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    SolidBrush bg(Color(255,15,29,49));
    g.FillRectangle(&bg,0,0,rc.right,rc.bottom);
    SolidBrush panel(Color(255,29,52,77));
    g.FillRectangle(&panel,18,60,300,445);
    label(g,L"DIGIMON HYPER COLOSSEUM",18,13,24,Color(255,238,204,95));
    label(g,L"Bo-1 ... Bo-300   |   Left / Right: browse",18,42,14,Color(255,200,218,235));
    if(cards.empty()) {
        label(g,L"No Bo cards in CSV.",32,120,18,Color(255,255,120,120));
        return;
    }
    const hc::CatalogCard& c=*cards[selected];
    const std::wstring id=widen(c.id);
    const std::wstring file=imageDir+id+L".jpg";
    Image picture(file.c_str());
    if(picture.GetLastStatus()==Ok && picture.GetWidth()>0 && picture.GetHeight()>0) {
        REAL ratio=(REAL)picture.GetWidth()/picture.GetHeight();
        REAL width=ratio*390;
        if(width>270)width=270;
        REAL height=width/ratio;
        g.DrawImage(&picture,30+(270-width)/2,70+(420-height)/2,width,height);
    } else {
        label(g,L"Image missing:",35,165,18,Color(255,255,150,125));
        label(g,file,35,200,13,Color(255,230,230,230));
        label(g,L"Run download_card_images_original.bat",35,245,12,Color(255,230,230,230));
    }
    label(g,id,350,85,30,Color(255,238,204,95));
    label(g,widen(c.name),350,135,21,Color(255,255,255,255));
    label(g,widen(c.kind),350,180,17,Color(255,200,218,235));
    label(g,widen(c.set),350,212,15,Color(255,200,218,235));
    label(g,L"Card "+std::to_wstring(selected+1)+L" / "+std::to_wstring(cards.size()),350,260,18,Color(255,238,204,95));
    label(g,L"Collection preview - rules not yet verified",350,310,14,Color(255,200,218,235));
    label(g,L"Arrow keys to select another card",350,350,15,Color(255,255,255,255));
}
static LRESULT CALLBACK wndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_KEYDOWN && !cards.empty()) {
        if(wp==VK_RIGHT)selected=(selected+1)%cards.size();
        else if(wp==VK_LEFT)selected=(selected+cards.size()-1)%cards.size();
        else return DefWindowProcW(hwnd,msg,wp,lp);
        InvalidateRect(hwnd,0,FALSE);
        return 0;
    }
    if(msg==WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc=BeginPaint(hwnd,&ps);
        RECT rc;GetClientRect(hwnd,&rc);
        draw(dc,rc);
        EndPaint(hwnd,&ps);
        return 0;
    }
    if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,LPWSTR,int show) {
    GdiplusStartupInput input;
    ULONG_PTR token=0;
    if(GdiplusStartup(&token,&input,0)!=Ok)return 1;
    hc::Result result=catalog.loadCSV("data/cards.csv");
    if(!result.ok){MessageBoxA(0,result.message.c_str(),"Catalog error",MB_OK|MB_ICONERROR);GdiplusShutdown(token);return 2;}
    cards=catalog.all();
    WNDCLASSW wc={};
    wc.lpfnWndProc=wndProc;wc.hInstance=inst;
    wc.lpszClassName=L"HC_CardViewer";
    wc.hCursor=LoadCursor(0,IDC_ARROW);
    RegisterClassW(&wc);
    HWND hwnd=CreateWindowW(wc.lpszClassName,L"Digimon Card Viewer - Bo-1 to Bo-300",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,850,560,0,0,inst,0);
    if(!hwnd){GdiplusShutdown(token);return 3;}
    ShowWindow(hwnd,show);
    MSG msg;
    while(GetMessageW(&msg,0,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    GdiplusShutdown(token);
    return (int)msg.wParam;
}
#endif
