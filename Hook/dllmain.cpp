// dllmain.cpp
#include "pch.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <MinHook.h>

#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

#if _WIN64
#pragma comment(lib, "libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// You must link MinHook library for your platform (libMinHook.x86.lib / libMinHook.x64.lib)
// Example (uncomment in your project or add to linker): 
// #pragma comment(lib, "libMinHook.x86.lib")

// Logging helper:
static FILE* g_log = nullptr;
static void Log(const char* fmt, ...)
{
    /*
    if (!g_log) {
        CHAR path[MAX_PATH];
        GetModuleFileNameA((HMODULE)&__ImageBase, path, MAX_PATH);
        // change extension to .log
        std::string s(path);
        auto p = s.find_last_of("\\/");
        std::string dir = (p == std::string::npos) ? "." : s.substr(0, p);
        std::string logfile = dir + "\\PvZHook.log";
        fopen_s(&g_log, logfile.c_str(), "a");
        if (!g_log) return;
    }
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    fprintf(g_log, "\n");
    fflush(g_log);
    va_end(ap);
    */
}

// forward decl for Getting DirectDrawCreate from ddraw.dll:
typedef HRESULT(WINAPI* DirectDrawCreate_t)(GUID FAR* lpGuid, void** lplpDD, IUnknown FAR* pUnkOuter);

// BLT signature (IDirectDrawSurface / IDirectDrawSurface2 style)
typedef HRESULT(WINAPI* Surface_Blt_t)(
    void* pSurface, // pointer to IDirectDrawSurface / IDirectDrawSurface2
    LPRECT dst,
    void* srcSurface,
    LPRECT srcRect,
    DWORD flags,
    LPDDBLTFX fx
    );

static Surface_Blt_t g_originalBlt = nullptr;

// Helper: get vtable pointer from COM object pointer
static inline void** GetVTable(void* obj) {
    if (!obj) return nullptr;
    return *reinterpret_cast<void***>(obj);
}

HRESULT WINAPI HookedBlt(void* pSurface, LPRECT dst, void* srcSurface, LPRECT srcRect, DWORD flags, LPDDBLTFX fx)
{
    // Call original first (safer) so game draws, then we draw UI on top.
    HRESULT hr = g_originalBlt(pSurface, dst, srcSurface, srcRect, flags, fx);

    // Obtain HDC from surface and draw GUI
    // Many IDirectDrawSurface implementations support GetDC/ReleaseDC (DirectDraw surfaces).
    // We call through vtable to get GetDC, index differs between IDirectDrawSurface versions.
    // But we can use the exported method by using the surface pointer's function pointer.
    // We'll attempt to call GetDC via vtable slot (index around 31 or 33 depending on header),
    // but the simpler way: Query the surface for HDC via COM call GetDC (slot 33 for older headers).
    // To avoid fragile index assumptions, we attempt two likely indexes and fallback.

    HDC hdc = nullptr;
    bool got = false;

    // Try a few common indices for GetDC in DirectDraw surface vtable:
    void** vtbl = GetVTable(pSurface);
    if (vtbl) {
        // Common indexes where GetDC may reside: 33, 34 (empirical). We'll try 33 and 34.
        typedef HRESULT(WINAPI* GetDC_t)(void* pThis, HDC* phdc);
        GetDC_t getdc = nullptr;
        // try index 33
        if (vtbl[33]) {
            getdc = reinterpret_cast<GetDC_t>(vtbl[33]);
            if (SUCCEEDED(getdc(pSurface, &hdc))) got = true;
        }
        if (!got && vtbl[34]) {
            getdc = reinterpret_cast<GetDC_t>(vtbl[34]);
            if (SUCCEEDED(getdc(pSurface, &hdc))) got = true;
        }
        // fallback: try QueryInterface for IDirectDrawSurface7->GetDC isn't portable; we stick to above tries.
    }

    if (got && hdc) {
        // Get cursor position relative to window owning DC
        HWND hwnd = WindowFromDC(hdc);
        POINT p;
        GetCursorPos(&p);
        if (hwnd) ScreenToClient(hwnd, &p);
        else {
            // fallback: assume full-screen coords
        }

        /*
        g_gui.BeginFrame(p.x, p.y, (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

        if (g_gui.Button(50, 50, 120, 35, "Click Me")) {
            MessageBoxA(hwnd, "Button Clicked!", "PvZHook", MB_OK | MB_ICONINFORMATION);
        }
        g_gui.Label(50, 100, "Hello PvZHook!");
        g_gui.EndFrame();

        g_gui.Render(hdc);
        */

        // Release DC via vtable method indices 34/33 or using ReleaseDC slot
        typedef HRESULT(WINAPI* ReleaseDC_t)(void* pThis, HDC hdc);
        ReleaseDC_t releasedc = nullptr;
        if (vtbl[34]) {
            releasedc = reinterpret_cast<ReleaseDC_t>(vtbl[34]);
            releasedc(pSurface, hdc);
        }
        else if (vtbl[33]) {
            // if we used 33 to get DC, maybe 33+1 is release; try 34
            if (vtbl[34]) {
                releasedc = reinterpret_cast<ReleaseDC_t>(vtbl[34]);
                releasedc(pSurface, hdc);
            }
            else {
                // last resort: call ReleaseDC WinAPI if we can find hwnd
                if (hwnd) ReleaseDC(hwnd, hdc);
            }
        }
    }
    else {
        // couldn't get HDC — skip drawing
    }

    return hr;
}

// Create a temporary DirectDraw primary surface and hook its Blt entry
DWORD WINAPI MainThread(LPVOID lpParam)
{
    Log("PvZHook: thread start.");

    // init MinHook
    if (MH_Initialize() != MH_OK) {
        Log("MH_Initialize failed");
        return 1;
    }

    // Load ddraw.dll and get DirectDrawCreate (export)
    HMODULE hDdraw = LoadLibraryA("ddraw.dll");
    if (!hDdraw) {
        Log("ddraw.dll not found");
        return 1;
    }
    auto pDirectDrawCreate = (DirectDrawCreate_t)GetProcAddress(hDdraw, "DirectDrawCreate");
    if (!pDirectDrawCreate) {
        Log("DirectDrawCreate not found");
        return 1;
    }

    // Create IDirectDraw
    void* pDD = nullptr;
    HRESULT hr = pDirectDrawCreate(NULL, &pDD, NULL);
    if (FAILED(hr) || !pDD) {
        Log("DirectDrawCreate failed: 0x%08X", (unsigned)hr);
        return 1;
    }
    Log("DirectDrawCreate OK: %p", pDD);

    // We will call CreateSurface on that pointer. Since we declared void*, use vtable call:
    // vtbl index for CreateSurface in IDirectDraw/IDirectDraw2 is usually 11 (but we'll find it).
    void** ddVtbl = *reinterpret_cast<void***>(pDD);
    if (!ddVtbl) {
        Log("No vtable for IDirectDraw");
        return 1;
    }

    // typedef for CreateSurface: HRESULT (WINAPI*)(void* pThis, DDSURFACEDESC*, void** ppSurface, IUnknown*)
    typedef HRESULT(WINAPI* CreateSurface_t)(void* pThis, void* desc, void** ppSurface, void* unk);
    CreateSurface_t createSurface = reinterpret_cast<CreateSurface_t>(ddVtbl[11]); // empirical index
    if (!createSurface) {
        Log("CreateSurface pointer missing");
        return 1;
    }

    // Prepare surface desc for primary
    struct DDSURFACEDESC2_local {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        int32_t  lPitch;
        uint32_t dwBackBufferCount;
        uint32_t dwRefreshRate;
        uint32_t dwAlphaBitDepth;
        uint32_t dwReserved;
        void* lpSurface;
        // we only need dwSize + dwFlags + ddsCaps for primary creation
        // keep rest zeroed
        uint8_t  padding[1024];
    };
    DDSURFACEDESC2_local desc{};
    desc.dwSize = sizeof(DDSURFACEDESC2_local);
    // DDSD_CAPS value
    desc.dwFlags = DDSD_CAPS;
    // embed caps into padding (not pretty but sufficient)
    // location of ddsCaps.dwCaps in a true DDSURFACEDESC2 is after width/height etc.
    // Instead of trying to craft the full struct, call IDirectDraw::SetCooperativeLevel and CreateSurface via COM normally.
    // Simpler fallback: call DirectDrawCreate to get IDirectDraw2, then call it via documented API — but that needs headers.
    // We'll try a different approach: Query for IDirectDraw2 via QueryInterface slot 3 on pDD.

    typedef HRESULT(WINAPI* QueryInterface_t)(void* pThis, const GUID& riid, void** ppvObj);
    QueryInterface_t qit = reinterpret_cast<QueryInterface_t>(ddVtbl[0]);

    // IID_IDirectDraw2: {B3A6F3E0-2D36-11CF-9B9C-00AA0062A9A6}  (well-known)
    // We'll use the well known GUID for IID_IDirectDraw2:
    static const GUID IID_IDirectDraw2 =
    { 0xB3A6F3E0, 0x2D36, 0x11CF, {0x9B,0x9C,0x00,0xAA,0x00,0x62,0xA9,0xA6} };

    void* pDD2 = nullptr;
    hr = qit(pDD, IID_IDirectDraw2, &pDD2);
    if (FAILED(hr) || !pDD2) {
        Log("QueryInterface(IDirectDraw2) failed: 0x%08X", (unsigned)hr);
        // Try to continue with original pDD as a fallback
        pDD2 = pDD;
    }
    else {
        Log("Got IDirectDraw2: %p", pDD2);
    }

    // Now call CreateSurface via pDD2 vtable index (11 is usually CreateSurface)
    void** dd2Vtbl = *reinterpret_cast<void***>(pDD2);
    if (!dd2Vtbl) {
        Log("No vtable on dd2");
        return 1;
    }
    createSurface = reinterpret_cast<CreateSurface_t>(dd2Vtbl[11]);
    if (!createSurface) {
        Log("createSurface missing");
        return 1;
    }

    // Build a minimal primary DDSURFACEDESC (use proper layout via DDSURFACEDESC2 from ddraw.h size)
    // We'll craft a small buffer with dwSize and DDSD_CAPS + DDSCAPS_PRIMARYSURFACE (at right offsets).
    // True DDSURFACEDESC2 layout: first DWORD dwSize; then dwFlags (DWORD); ... then ddCaps (DDSCAPS2) around offset ~76.
    // We'll allocate a zeroed buffer of sizeof(DDSURFACEDESC2) and set what we need.
    const size_t DDS_DESC_SIZE = 124; // well-known size for DDSURFACEDESC2 (old)
    std::vector<uint8_t> raw(DDS_DESC_SIZE);
    *(uint32_t*)&raw[0] = (uint32_t)DDS_DESC_SIZE;            // dwSize
    *(uint32_t*)&raw[4] = DDSD_CAPS;                         // dwFlags
    // ddCaps.dwCaps at offset 76 (0x4C) in DDSURFACEDESC2
    const size_t CAPS_OFFSET = 0x4C;
    *(uint32_t*)&raw[CAPS_OFFSET] = DDSCAPS_PRIMARYSURFACE;

    void* pPrimary = nullptr;
    hr = createSurface(pDD2, raw.data(), &pPrimary, nullptr);
    if (FAILED(hr) || !pPrimary) {
        Log("CreateSurface(primary) failed: 0x%08X", (unsigned)hr);
        return 1;
    }
    Log("Primary surface created: %p", pPrimary);

    // Get vtable for primary surface
    void** surfVtbl = *reinterpret_cast<void***>(pPrimary);
    if (!surfVtbl) {
        Log("Primary surface vtable missing");
        return 1;
    }

    // Assume Blt is at index 5 (common for many versions). Grab pointer and hook it.
    const int BLT_INDEX = 5;
    void* pBlt = surfVtbl[BLT_INDEX];
    if (!pBlt) {
        Log("Blt pointer not found at vtable index %d", BLT_INDEX);
        return 1;
    }
    Log("Blt pointer found: %p", pBlt);

    // Create MinHook trampoline
    if (MH_CreateHook(pBlt, &HookedBlt, reinterpret_cast<LPVOID*>(&g_originalBlt)) != MH_OK) {
        Log("MH_CreateHook failed");
        return 1;
    }
    if (MH_EnableHook(pBlt) != MH_OK) {
        Log("MH_EnableHook failed");
        return 1;
    }
    Log("Hook installed on Blt.");

    // Keep thread alive (we don't exit so DLL stays loaded). The hook will run in game threads.
    // We don't want to busy-wait; sleep long but allow process to continue.
    for (;;) Sleep(10000);

    return 0;
}

// Dll entry
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // spawn thread to set up hooks
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        if (g_log) { fclose(g_log); g_log = nullptr; }
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
