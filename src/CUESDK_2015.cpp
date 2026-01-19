#define _UNICODE
#define UNICODE
#include <Windows.h>
#include <Psapi.h>
#include "iCUESDK.h"

HMODULE g_realDLL = nullptr;

#define RESOLVE(_Name)                      \
    real_##_Name = (decltype(real_##_Name)) \
        GetProcAddress(g_realDLL, #_Name)   \

template<typename T>
T Resolve(const char* name) {
    if (!g_realDLL)
        g_realDLL = LoadLibrary(L"CUESDK_ORIGIN.dll");

    void* ptr = (void*)GetProcAddress(g_realDLL, name);
    if (!ptr) return nullptr;
    return reinterpret_cast<T>(ptr);
}

#define FORWARD_FUNC(_ReturnType, _Name, _Params, _Args) \
extern "C" __declspec(dllexport)                         \
_ReturnType _Name _Params {                              \
    using FnType = _ReturnType(*)_Params;                \
    static FnType realFunc = nullptr;                    \
                                                         \
    if (!realFunc)                                       \
        realFunc = Resolve<FnType>(#_Name);              \
                                                         \
    return realFunc _Args;                               \
}                                                        \

FORWARD_FUNC(CorsairError, CorsairConnect, (CorsairSessionStateChangedHandler handler, void* context), (handler, context))
FORWARD_FUNC(CorsairError, CorsairGetSessionDetails, (CorsairSessionDetails* details), (details))
FORWARD_FUNC(CorsairError, CorsairDisconnect, (void), ())

bool IsTerrariaLoaded() {
    HMODULE hDLL[1024];
    DWORD cbNeeded;
    DWORD dwProcModules = EnumProcessModules(
        GetCurrentProcess(),
        hDLL,
        sizeof(hDLL),
        &cbNeeded);

    if (!dwProcModules)
        return false;

    unsigned int count = cbNeeded / sizeof(HMODULE);
    for (unsigned int i = 0; i < count; i++) {
        WCHAR modName[MAX_PATH] = {0};

        if (!GetModuleFileName(hDLL[i], modName, _countof(modName)))
            continue;
        if (wcsstr(modName, L"Terraria.exe"))
            return true;
    }
    return false;
}

void CALLBACK LoaderThread(PTP_CALLBACK_INSTANCE, void*, PTP_WORK) {
    while (!IsTerrariaLoaded())
        Sleep(16);

    LoadLibrary(L"TML.dll");
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hInstDLL);

            PTP_WORK pwk = CreateThreadpoolWork(
                LoaderThread,
                nullptr,
                nullptr);
            SubmitThreadpoolWork(pwk);
            break;
        }
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
