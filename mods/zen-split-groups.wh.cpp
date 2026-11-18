// ==WindhawkMod==
// @id              zen-split-groups
// @name            Zen · 开始菜单分组
// @name:zh-CN      Zen · 开始菜单分组
// @description     自定义类别与应用，不更改开始菜单外观。
// @description:zh-CN 自定义类别与应用，不更改开始菜单外观。
// @version         1.2.0
// @author          Local category customization
// @github          https://github.com/Liset999
// @include         StartMenuExperienceHost.exe
// @architecture    x86-64
// @compilerOptions -lruntimeobject
// ==/WindhawkMod==

// Source code is published under The GNU General Public License v3.0.
//
// For bug reports and feature requests, please open an issue here:
// https://github.com/Liset999/ZenDesktop/issues
//
// Based on "Windows 11 Start Menu Styler" by m417z (https://github.com/m417z),
// licensed under GPL-3.0. The original mod is developed here:
// https://github.com/m417z/my-windhawk-mods

// ==WindhawkModReadme==
/*
原生开始菜单 → 全部 → 类别。打开「使用我的分组」，收起预览最多四个独立应用，按完整 AppID 单独分组，展开后直接显示应用；未指定的快捷方式留在末尾的「其他」。套件文件夹仅在开始菜单的应用列表中展开，不改动磁盘文件。每行填一个完整 AppID（可用 Get-StartApps 查询）。只适配已核对的 x64 StartMenu.dll 2608.26001.0.0；不修改系统分类记录。仅有空组时不保证显示卡片。
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- clearAutoCategories: false
  $name: "使用我的分组（清空原有自动分类）"
  $description: "开启后，所有未指定的应用，包括新安装的应用，默认归入其他。关闭即可恢复自动分类，不删除你的规则。"
- myGroups:
  - - title: ""
      $name: "自定义组名"
    - ids: [""]
      $name: "组内应用（每项一个完整 AppID）"
  $name: "我的分组"
  $description: "点击 + 添加组，给组起名并添加应用；最多 31 行分组。使用 PowerShell 的 Get-StartApps 查看 AppID。保存后重新登录可重建菜单。"
*/
// ==/WindhawkModSettings==
#include <windows.h>
#include <winstring.h>


#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace PersonalStart {
constexpr unsigned kLastCategory = 31;
constexpr size_t kMaximumRules = 512;
constexpr unsigned kNoOverride = 0xffffffffu;

std::wstring_view Trim(std::wstring_view s) {
    constexpr std::wstring_view spaces = L" \t\r\n";
    auto first = s.find_first_not_of(spaces);
    if (first == s.npos) return {};
    return s.substr(first, s.find_last_not_of(spaces) - first + 1);
}

unsigned ParseCategory(std::wstring_view s) {
    if (s.empty() || s.size() > 2) return kNoOverride;
    unsigned value = 0;
    for (wchar_t c : s) {
        if (c < L'0' || c > L'9') return kNoOverride;
        value = value * 10 + (c - L'0');
    }
    return value <= kLastCategory ? value : kNoOverride;
}

bool SameId(std::wstring_view a, std::wstring_view b) {
    return !a.empty() && a.size() == b.size() && a.size() <= 4096 &&
           CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(),
                                static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
}

struct AppRule {
    std::wstring id;
    unsigned category;
};
struct Rules {
    std::vector<AppRule> apps;
    std::array<std::wstring, kLastCategory + 1> captions;
    bool hasCaptions = false;
    bool clearAutoCategories = false;
    bool personal = false;
};
std::shared_ptr<const Rules> g_rules;
SRWLOCK g_rulesLock = SRWLOCK_INIT;

std::shared_ptr<const Rules> ReadRules() {
    AcquireSRWLockShared(&g_rulesLock);
    auto snapshot = g_rules;
    ReleaseSRWLockShared(&g_rulesLock);
    return snapshot;
}

using Setting = std::unique_ptr<const wchar_t, decltype(&Wh_FreeStringSetting)>;

bool ReloadRules() {
    try {
        auto next=std::make_shared<Rules>();
        next->personal=next->clearAutoCategories=Wh_GetIntSetting(L"clearAutoCategories")!=0;
        if(next->personal) for(int g=0; g<31; ++g){
            Setting label(Wh_GetStringSetting(L"myGroups[%d].title",g),Wh_FreeStringSetting);
            auto title=Trim(label ? label.get() : L"");
            if(title.empty() || title.size()>64) continue;
            unsigned slot=static_cast<unsigned>(g+1);
            next->captions[slot]=title; next->hasCaptions=true;
            for(int i=0;i<512;++i){
                Setting id(Wh_GetStringSetting(L"myGroups[%d].ids[%d]",g,i),Wh_FreeStringSetting);
                if(!id || !*id.get())break;
                auto value=Trim(id.get()); if(!value.empty() && value.size()<=4096)next->apps.push_back({std::wstring(value),slot});
            }
        }
        std::shared_ptr<const Rules> snapshot(std::move(next));
        AcquireSRWLockExclusive(&g_rulesLock);
        g_rules.swap(snapshot);
        ReleaseSRWLockExclusive(&g_rulesLock);
        return true;
    } catch (...) {
        Wh_Log(L"Could not reload settings; previous rules were kept");
        return false;
    }
}

std::wstring_view View(HSTRING str) {
    UINT32 length = 0;
    PCWSTR data = WindowsGetStringRawBuffer(str, &length);
    return {data, length};
}

unsigned CategoryFromResource(std::wstring_view key) {
    constexpr std::wstring_view prefix = L"AllApps_Category_";
    if (key == L"AllApps_Category_Other") return 0;
    if (!key.starts_with(prefix)) return kNoOverride;
    const auto suffix = key.substr(prefix.size());
    if (suffix.empty() || suffix.front() == L'0') return kNoOverride;
    return ParseCategory(suffix);
}

// The Windows x64 ABI passes these nontrivial, one-pointer C++/WinRT
// values via caller-allocated slots. The original routines retain ownership
// of the input Tile/TileId slots and initialize the output HSTRING slot.
using GetCategoryFn = unsigned (*)(void* provider, void* tileSlot);
using GetAppIdFn = HSTRING* (*)(HSTRING* result, void* tileIdSlot);
using ResolveStringFn = HSTRING* (*)(void* resolverSlot, HSTRING* result, const HSTRING* key);
GetCategoryFn g_originalCategory;
GetAppIdFn g_originalAppId;
ResolveStringFn g_originalResolveString;
std::atomic<bool> g_ready{false};

struct Capture;
thread_local Capture* g_capture;
struct Capture {
    const Rules& rules;
    Capture* previous = g_capture;
    unsigned category = kNoOverride;
    explicit Capture(const Rules& snapshot) : rules(snapshot) { g_capture = this; }
    ~Capture() { g_capture = previous; }
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
};

HSTRING* CaptureAppId(HSTRING* result, void* tileIdSlot) {
    auto* out = g_originalAppId(result, tileIdSlot);
    if (auto* capture = g_capture; capture && out) {
        const auto id = View(*out);
        capture->category = kNoOverride;
        // Last matching rule wins, without case-sensitive aliases or wildcards.
        for (auto it = capture->rules.apps.rbegin(); it != capture->rules.apps.rend(); ++it) {
            if (SameId(id, it->id)) {
                capture->category = it->category;
                break;
            }
        }
    }
    return out;
}

unsigned OverrideCategory(void* provider, void* tileSlot) {
    if (!g_ready.load()) return g_originalCategory(provider, tileSlot);
    const auto rules = ReadRules();
    if (!rules || (rules->apps.empty() && !rules->clearAutoCategories)) {
        return g_originalCategory(provider, tileSlot);
    }
    Capture capture(*rules);
    const unsigned original = g_originalCategory(provider, tileSlot);
    if (capture.category != kNoOverride) return capture.category;
    return rules->clearAutoCategories ? 0u : original;  // Native "Other" category.
}

using CombineFn = void (*)(void*, void*);
CombineFn g_originalCombine;
void KeepPersonalGroups(void* self, void* groups) {
    const auto rules = g_ready.load() ? ReadRules() : nullptr;
    // The category map is already populated. Let the unmodified rank/render
    // stages handle it, but don't fold one- and two-app personal groups.
    if (!rules || !rules->personal) g_originalCombine(self, groups);
}

// The native classifier assigns a whole suite to the first matching child.
// In personal mode build ordinary app tiles instead, before categorization.
// Both names are properties of the list model, not filesystem metadata.
using SuiteProperty = HSTRING* (*)(void*, HSTRING*);
SuiteProperty g_suiteName, g_suiteSortName;
bool PersonalMode() {
    const auto rules = g_ready.load() ? ReadRules() : nullptr;
    return rules && rules->personal;
}
HSTRING* SuiteName(void* tile, HSTRING* out) {
    if (!PersonalMode()) return g_suiteName(tile, out);
    *out = nullptr;
    return out;
}
HSTRING* SuiteSortName(void* tile, HSTRING* out) {
    if (!PersonalMode()) return g_suiteSortName(tile, out);
    *out = nullptr;
    return out;
}

// Pinned build: the returned MSVC vector stores a category and an owning
// three-pointer vector per record. Permute intact records without destroying
// or duplicating any owner; never change the ordering among personal groups.
using RankFn = void* (*)(void*, void*, void*, void*);
RankFn g_rank;
void* RankOtherLast(void* self, void* out, void* categories, void* resolver) {
    auto* ret = g_rank(self, out, categories, resolver);
    if (ret != out || !out || !PersonalMode()) return ret;
    uintptr_t v[3];
    std::memcpy(v, out, sizeof(v));
    constexpr size_t stride = 32;
    if (!v[0] || v[0] > v[1] || v[1] > v[2]) return ret;
    const auto bytes = v[1] - v[0];
    if (bytes % stride || bytes > stride * 256 || bytes < 2 * stride) return ret;
    auto* base = reinterpret_cast<uint8_t*>(v[0]);
    for (size_t off = 0; off < bytes - stride; off += stride) {
        uint32_t category;
        std::memcpy(&category, base + off, sizeof(category));
        if (category != 0) continue;
        std::array<uint8_t, stride> saved;
        std::memcpy(saved.data(), base + off, stride);
        std::memmove(base + off, base + off + stride, bytes - off - stride);
        std::memcpy(base + bytes - stride, saved.data(), stride);
        break;
    }
    return ret;
}

HSTRING* OverrideCaption(void* resolverSlot, HSTRING* result, const HSTRING* key) {
    if (g_ready.load() && key && *key) {
        const auto id = CategoryFromResource(View(*key));
        if (id != kNoOverride) {
            const auto rules = ReadRules();
            if (rules && rules->hasCaptions && !rules->captions[id].empty()) {
                const auto& text = rules->captions[id];
                HSTRING caption = nullptr;
                if (SUCCEEDED(WindowsCreateString(text.data(), static_cast<UINT32>(text.size()), &caption))) {
                    *result = caption;
                    return result;
                }
            }
        }
    }
    return g_originalResolveString(resolverSlot, result, key);
}

// Checked against Microsoft public symbols and disassembly for the exact
// StartMenu.dll on Windows 11 25H2, 26200.9711 (file version 2608.26001.0.0).
// Require its PDB GUID + Age and each of the verified entry-point prologues.
constexpr DWORD kImageSize = 0x4dd000;
constexpr DWORD kTimestamp = 0x6a8f587a;
constexpr DWORD kCodeViewRva = 0x42b628;
constexpr uint8_t kCodeViewId[] = {
    0x52, 0x53, 0x44, 0x53, 0xa3, 0x8b, 0x2e, 0x93,
    0x3a, 0xfa, 0x42, 0x41, 0x94, 0x98, 0xe5, 0xf6,
    0x42, 0x6a, 0x43, 0xe6, 0x01, 0x00, 0x00, 0x00,
};
constexpr DWORD kGetCategoryRva = 0x147fc8;
constexpr uint8_t kGetCategoryPrefix[] = {0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x57,0x48,0x8b,0xec,0x48,0x83,0xec,0x70,0x48};
constexpr DWORD kGetAppIdRva = 0x62fc;
constexpr uint8_t kGetAppIdPrefix[] = {0x48,0x89,0x54,0x24,0x10,0x55,0x53,0x57,0x48,0x8b,0xec,0x48,0x83,0xec,0x30,0x48};
constexpr DWORD kResolveStringRva = 0x164f0;
constexpr uint8_t kResolveStringPrefix[] = {0x4c,0x8b,0xdc,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0x05,0x41,0xeb,0x46,0x00,0x48};

constexpr DWORD kCombineRva = 0x14a3e4;
constexpr uint8_t kCombinePrefix[] = {0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20,0x55};

constexpr DWORD kSuiteName = 0xd2db4, kSuiteSortName = 0xd2e3c, kRank = 0x14ac30;
constexpr uint8_t kSuitePrefix[] = {0x4c,0x8b,0xdc,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0x05,0x7d,0x22,0x3b,0x00,0x48};
constexpr uint8_t kSortPrefix[] = {0x4c,0x8b,0xdc,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0x05,0xf5,0x21,0x3b,0x00,0x48};
// UpdateSummaryAppList has two native layouts on this build. Both choose
// a regular fourth app when this particular size query returns four, and a
// miniature overflow tile otherwise. Limit only that *presentation decision*;
// never truncate the vector or change Size() seen by the expanded apps grid.
constexpr DWORD kAppVectorSize = 0x96188;
constexpr uint8_t kSizePrefix[] = {0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x7c,0x24,0x18,0x55,0x48,0x8b,0xec,0x48,0x83};
constexpr DWORD kSummarySizeCalls[] = {0x113e6e,0x11434b};
constexpr uint8_t kSummaryCallBytes[][12] = {
    {0xe8,0x15,0x23,0xf8,0xff,0x83,0xf8,0x04,0x0f,0x85,0x97,0x00},
    {0xe8,0x38,0x1e,0xf8,0xff,0x49,0x8b,0xce,0x83,0xf8,0x04,0x75}};
using SizeFn = uint32_t (*)(void*);
SizeFn g_appVectorSize;
uintptr_t g_startBase;
uint32_t PreviewSize(void* vec) {
    auto caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - g_startBase;
    const auto n = g_appVectorSize(vec);
    if (n > 4 && (caller == kSummarySizeCalls[0]+5 || caller == kSummarySizeCalls[1]+5) && PersonalMode()) return 4;
    return n;
}
bool CheckImage(HMODULE module) {
    const auto* base = reinterpret_cast<const uint8_t*>(module);
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
        dos->e_lfanew > 4096 - static_cast<LONG>(sizeof(IMAGE_NT_HEADERS64))) return false;
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    return nt->Signature == IMAGE_NT_SIGNATURE && nt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
           nt->FileHeader.TimeDateStamp == kTimestamp && nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
           nt->OptionalHeader.SizeOfImage == kImageSize &&
           std::memcmp(base + kAppVectorSize,kSizePrefix,sizeof(kSizePrefix)) == 0 &&
           std::memcmp(base + kSummarySizeCalls[0],kSummaryCallBytes[0],12) == 0 &&
           std::memcmp(base + kSummarySizeCalls[1],kSummaryCallBytes[1],12) == 0 &&
           std::memcmp(base + kRank, kCombinePrefix, sizeof(kCombinePrefix)) == 0 &&
           std::memcmp(base + kSuiteName, kSuitePrefix, sizeof(kSuitePrefix)) == 0 &&
           std::memcmp(base + kSuiteSortName, kSortPrefix, sizeof(kSortPrefix)) == 0 &&
           std::memcmp(base + kCombineRva, kCombinePrefix, sizeof(kCombinePrefix)) == 0 &&
           std::memcmp(base + kCodeViewRva, kCodeViewId, sizeof(kCodeViewId)) == 0 &&
           std::memcmp(base + kGetCategoryRva, kGetCategoryPrefix, sizeof(kGetCategoryPrefix)) == 0 &&
           std::memcmp(base + kGetAppIdRva, kGetAppIdPrefix, sizeof(kGetAppIdPrefix)) == 0 &&
           std::memcmp(base + kResolveStringRva, kResolveStringPrefix, sizeof(kResolveStringPrefix)) == 0;
}

template <class Fn>
bool Hook(void* target, Fn replacement, Fn& original) {
    // Windhawk's Detours backend retains this slot through apply AND detach.
    // Every caller supplies a separate module-lifetime function pointer.
    return Wh_SetFunctionHook(target, reinterpret_cast<void*>(replacement),
                              reinterpret_cast<void**>(&original)) != FALSE;
}

bool QueueStartHooks(HMODULE module) {
    if (!CheckImage(module)) {
        Wh_Log(L"Unsupported StartMenu.dll; no category hooks installed");
        return false;
    }
    auto* base = reinterpret_cast<uint8_t*>(module);
    g_startBase = reinterpret_cast<uintptr_t>(base);
    if (!Hook(base + kAppVectorSize, PreviewSize, g_appVectorSize) ||
        !Hook(base + kGetAppIdRva, CaptureAppId, g_originalAppId) ||
        !Hook(base + kSuiteName, SuiteName, g_suiteName) ||
        !Hook(base + kSuiteSortName, SuiteSortName, g_suiteSortName) ||
        !Hook(base + kRank, RankOtherLast, g_rank) ||
        !Hook(base + kCombineRva, KeepPersonalGroups, g_originalCombine) ||
        !Hook(base + kGetCategoryRva, OverrideCategory, g_originalCategory) ||
        !Hook(base + kResolveStringRva, OverrideCaption, g_originalResolveString)) {
        Wh_Log(L"Could not register every category hook; overrides remain disabled");
        return false;
    }
    return true;
}

std::mutex g_installMutex;
bool g_installAttempted = false;
bool g_initialHooksQueued = false;
std::atomic<bool> g_canApply{false};
using LoadLibraryFn = decltype(&LoadLibraryExW);
LoadLibraryFn g_originalLoadLibrary;

void InstallInLoadedModule(HMODULE module) {
    if (!g_canApply.load() || !module) return;
    // Avoid taking the lock for unrelated modules (including other DLLs
    // Windhawk may load while it registers the hooks).
    if (GetModuleHandleW(L"StartMenu.dll") != module) return;
    std::unique_lock lock(g_installMutex, std::try_to_lock);
    if (!lock || !g_canApply.load() || g_installAttempted) return;
    g_installAttempted = true;
    if (QueueStartHooks(module) && Wh_ApplyHookOperations()) {
        g_ready.store(true);
        Wh_Log(L"Category overrides ready (delayed load)");
    }
}

HMODULE WINAPI AfterLoadLibrary(LPCWSTR filename, HANDLE file, DWORD flags) {
    HMODULE module = g_originalLoadLibrary(filename, file, flags);
    if (module && filename) {
        std::wstring_view name(filename);
        const auto separator = name.find_last_of(L"/\\");
        if (separator != name.npos) name.remove_prefix(separator + 1);
        if (SameId(name, L"StartMenu.dll")) InstallInLoadedModule(module);
    }
    return module;
}

BOOL Init() {
    if (!ReloadRules()) return FALSE;
    if (HMODULE module = GetModuleHandleW(L"StartMenu.dll")) {
        g_installAttempted = true;
        g_initialHooksQueued = QueueStartHooks(module);
        return g_initialHooksQueued ? TRUE : FALSE;
    }
    HMODULE kernelBase = GetModuleHandleW(L"kernelbase.dll");
    if (!kernelBase) return FALSE;
    auto target = GetProcAddress(kernelBase, "LoadLibraryExW");
    return target && Hook(reinterpret_cast<void*>(target), AfterLoadLibrary, g_originalLoadLibrary);
}

void AfterInit() {
    if (g_initialHooksQueued) {
        g_ready.store(true);
        Wh_Log(L"Category overrides ready");
    } else {
        g_canApply.store(true);
        InstallInLoadedModule(GetModuleHandleW(L"StartMenu.dll"));
    }
}

void SettingsChanged() {
    ReloadRules();
}

void BeforeUninit() {
    g_canApply.store(false);
    std::lock_guard lock(g_installMutex);
    g_ready.store(false);
}

} // namespace PersonalStart
BOOL Wh_ModInit(){return PersonalStart::Init();}
void Wh_ModAfterInit(){PersonalStart::AfterInit();}
void Wh_ModBeforeUninit(){PersonalStart::BeforeUninit();}
void Wh_ModSettingsChanged(){PersonalStart::SettingsChanged();}
