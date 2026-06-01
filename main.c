#define COBJMACROS
#include <windows.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <strsafe.h>
#include "config.h"

IMFMediaEngine* g_pMediaEngine = NULL;

BOOL EnsureSingleInstance(void) {
    CreateMutexW(NULL, TRUE, MUTEX_NAME);
    return (GetLastError() != ERROR_ALREADY_EXISTS);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND* pWorkerw = (HWND*)lParam;
    if (FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL)) {
        *pWorkerw = FindWindowExW(NULL, hwnd, L"WorkerW", NULL);
    }
    return TRUE;
}

HWND GetWorkerW(void) {
    HWND progman = FindWindowW(L"Progman", NULL);
    SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, NULL);
    HWND workerw = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&workerw);
    return workerw;
}

typedef struct {
    IMFMediaEngineNotify Vtbl;
    long m_cRef;
} MediaEngineNotifyC;

STDMETHODIMP QueryInterfaceC(IMFMediaEngineNotify* thisPtr, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMFMediaEngineNotify)) {
        *ppvObject = thisPtr;
        thisPtr->lpVtbl->AddRef(thisPtr);
        return S_OK;
    }
    return E_NOINTERFACE;
}
STDMETHODIMP_(ULONG) AddRefC(IMFMediaEngineNotify* thisPtr) {
    return InterlockedIncrement(&((MediaEngineNotifyC*)thisPtr)->m_cRef);
}
STDMETHODIMP_(ULONG) ReleaseC(IMFMediaEngineNotify* thisPtr) {
    ULONG cRef = InterlockedDecrement(&((MediaEngineNotifyC*)thisPtr)->m_cRef);
    if (cRef == 0) HeapFree(GetProcessHeap(), 0, thisPtr);
    return cRef;
}
STDMETHODIMP EventNotifyC(IMFMediaEngineNotify* thisPtr, DWORD event, DWORD_PTR param1, DWORD param2) {
    if (event == MF_MEDIA_ENGINE_EVENT_ENDED && g_pMediaEngine) {
        IMFMediaEngine_SetCurrentTime(g_pMediaEngine, 0.0);
        IMFMediaEngine_Play(g_pMediaEngine);
    }
    return S_OK;
}
static IMFMediaEngineNotifyVtbl g_NotifyVtbl = { QueryInterfaceC, AddRefC, ReleaseC, EventNotifyC };

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!EnsureSingleInstance()) return 0;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

#if defined(_MSC_VER)
    MFStartup(MF_VERSION, MFSTARTUP_FULL);
#else
    MFStartup(MF_VERSION, MFSTARTUP_FULL); 
#endif

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WEBM_WINDOW_CLASS;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, WEBM_WINDOW_CLASS, L"Animated Wallpapers Reimagined", 
                               WS_POPUP | WS_VISIBLE, 0, 0, sw, sh, NULL, NULL, hInstance, NULL);

    HWND workerw = GetWorkerW();
    if (workerw) SetParent(hwnd, workerw);

    IMFMediaEngineClassFactory* pFactory = NULL;
    IMFAttributes* pAttributes = NULL;
    MediaEngineNotifyC* pNotify = (MediaEngineNotifyC*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MediaEngineNotifyC));
    pNotify->Vtbl.lpVtbl = &g_NotifyVtbl;
    pNotify->m_cRef = 1;

    CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFMediaEngineClassFactory, (void**)&pFactory);
    MFCreateAttributes(&pAttributes, 3);
    
    IMFAttributes_SetUnknown(pAttributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)pNotify);
    IMFAttributes_SetUINT64(pAttributes, &MF_MEDIA_ENGINE_PLAYBACK_HWND, (UINT64)hwnd);
    IMFAttributes_SetUINT32(pAttributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);

    IMFMediaEngineClassFactory_CreateInstance(pFactory, 0, pAttributes, &g_pMediaEngine);

    wchar_t videoPath[MAX_PATH];
    GetModuleFileNameW(NULL, videoPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(videoPath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    StringCchCatW(videoPath, MAX_PATH, DEFAULT_VIDEO_NAME);

    BSTR bstrURL = SysAllocString(videoPath);
    IMFMediaEngine_SetSource(g_pMediaEngine, bstrURL);
    IMFMediaEngine_Play(g_pMediaEngine);
    SysFreeString(bstrURL);

    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_pMediaEngine) IUnknown_Release(g_pMediaEngine);
    IUnknown_Release(pAttributes);
    IUnknown_Release(pFactory);

    MFShutdown();
    CoUninitialize();
    return 0;
}