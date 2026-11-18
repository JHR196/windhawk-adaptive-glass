// ==WindhawkMod==
// @id              zen-split-desktop-icons
// @name            Zen · 桌面图标快捷键
// @description     用自定义全局组合键切换桌面图标，不接管双击。
// @version         1.0.0
// @author          Local desktop customization
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -luser32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
默认 Ctrl+Alt+H。每按一次切换一次，与桌面右键 → 查看 → 显示桌面图标是同一开关，不会删除文件。停用后保留 Windows 当前显示状态。允许 Ctrl、Alt、Shift、Win 与一个 A-Z、0-9 或 F1-F24 主键组合（F12 为系统保留）。设置无效或新键已占用时保留本次运行中原先有效的快捷键；原因可在 Mod 日志查看。
*/
// ==/WindhawkModReadme==
// ==WindhawkModSettings==
/*
- hotkey: Ctrl+Alt+H
  $name: 切换桌面图标的全局快捷键
  $description: 例如 Ctrl+Alt+H 或 Ctrl+Shift+F8。请至少使用一个修饰键，避免与其他软件冲突。
*/
// ==/WindhawkModSettings==
#include <windows.h>
#include <string>
#include <string_view>

namespace {
HANDLE events[2]{}, worker{}; // stop, rebind
struct Binding { UINT mods{}, key{}; };
bool Parse(std::wstring_view value, Binding& b) {
    if(value.size()>80) return false;
    for (;;) {
        auto end=value.find(L'+'); auto t=value.substr(0,end);
        auto start=t.find_first_not_of(L" \t"), last=t.find_last_not_of(L" \t");
        if(start==t.npos) return false;
        std::wstring s(t.substr(start,last-start+1));
        for(auto& c:s) if(c>=L'a' && c<=L'z') c-=32;
        UINT m=s==L"CTRL"?MOD_CONTROL:s==L"ALT"?MOD_ALT:s==L"SHIFT"?MOD_SHIFT:s==L"WIN"?MOD_WIN:0;
        if(m) { if(b.mods&m) return false; b.mods|=m; }
        else {
            if(b.key) return false;
            if(s.size()==1 && ((s[0]>=L'A'&&s[0]<=L'Z')||(s[0]>=L'0'&&s[0]<=L'9'))) b.key=s[0];
            else if(s.size()>=2 && s.size()<=3 && s[0]==L'F' && s[1]!=L'0') {
                UINT n=0; for(size_t i=1;i<s.size();++i){if(s[i]<L'0'||s[i]>L'9')return false;n=n*10+s[i]-L'0';}
                if(n<1||n>24||n==12) return false; b.key=VK_F1+n-1;
            } else return false;
        }
        if(end==value.npos) break; value.remove_prefix(end+1);
    }
    return b.mods && b.key;
}
DWORD Owner(HWND w){DWORD p{}; if(w)GetWindowThreadProcessId(w,&p);return p;}
void Toggle() {
    HWND shell=GetShellWindow(); if(!shell || Owner(shell)!=GetCurrentProcessId()) return;
    HWND view=FindWindowExW(shell,nullptr,L"SHELLDLL_DefView",nullptr);
    for(HWND host=nullptr;!view && (host=FindWindowExW(nullptr,host,L"WorkerW",nullptr));)
        if(Owner(host)==GetCurrentProcessId()) view=FindWindowExW(host,nullptr,L"SHELLDLL_DefView",nullptr);
    if(view && Owner(view)==GetCurrentProcessId())
        PostMessageW(view,WM_COMMAND,MAKEWPARAM(0x7402,0),0); // desktop's View > Show desktop icons
}
DWORD WINAPI Run(void*) {
    MSG msg; PeekMessageW(&msg,nullptr,0,0,PM_NOREMOVE);
    HWND shell; while(!(shell=GetShellWindow())) if(WaitForSingleObject(events[0],250)!=WAIT_TIMEOUT) return 0;
    if(Owner(shell)!=GetCurrentProcessId()) return 0; // ignore separate folder processes
    Binding old{}; int active=0; bool bind=true;
    for(;;){
        if(bind){
            PCWSTR setting=Wh_GetStringSetting(L"hotkey"); Binding b{};
            bool ok=setting && Parse(setting,b); if(setting)Wh_FreeStringSetting(setting);
            if(!ok)Wh_Log(L"Invalid hotkey; existing binding unchanged");
            else if(b.mods!=old.mods || b.key!=old.key){
                int next=active==1?2:1;
                if(RegisterHotKey(nullptr,next,b.mods|MOD_NOREPEAT,b.key)){
                    if(active)UnregisterHotKey(nullptr,active); active=next;old=b;Wh_Log(L"Desktop icon hotkey registered");
                }else Wh_Log(L"Hotkey registration failed (%u); existing binding unchanged",GetLastError());
            }
            bind=false;
        }
        DWORD w=MsgWaitForMultipleObjects(2,events,FALSE,INFINITE,QS_ALLINPUT);
        if(w==WAIT_OBJECT_0 || w==WAIT_FAILED)break;
        if(w==WAIT_OBJECT_0+1){bind=true;continue;}
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE))
            if(active && msg.message==WM_HOTKEY && msg.wParam==static_cast<WPARAM>(active))Toggle();
    }
    if(active)UnregisterHotKey(nullptr,active);return 0;
}
void Close(){ for(auto& e:events)if(e){CloseHandle(e);e=nullptr;} }
}
BOOL Wh_ModInit(){events[0]=CreateEventW(nullptr,TRUE,FALSE,nullptr);events[1]=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(events[0]&&events[1])worker=CreateThread(nullptr,0,Run,nullptr,0,nullptr);
    if(!worker){Close();return FALSE;}return TRUE;}
void Wh_ModSettingsChanged(){SetEvent(events[1]);}
void Wh_ModUninit(){SetEvent(events[0]);WaitForSingleObject(worker,INFINITE);CloseHandle(worker);Close();}
