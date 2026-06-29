#define COBJMACROS
#include <windows.h>
#include <mfapi.h>
#include <mfmediaengine.h>
#include <strsafe.h>
#include "config.h"

#define OXOP_MAX_MONITORS 16

HWND g_hwnds[OXOP_MAX_MONITORS];
IMFMediaEngine* g_engines[OXOP_MAX_MONITORS];
IMFAttributes* g_attributes[OXOP_MAX_MONITORS];
int g_monitorCount = 0;

HINSTANCE g_hInstance = NULL;
IMFMediaEngineClassFactory* g_pFactory = NULL;
wchar_t g_videoPath[MAX_PATH];

BOOL EnsureSingleInstance(void) {
    CreateMutexW(NULL, TRUE, MUTEX_NAME);
    return (GetLastError() != ERROR_ALREADY_EXISTS);
}

BOOL CALLBACK KillOldInstancesProc(HWND hwnd, LPARAM lParam) {
    wchar_t className[256];
    if (GetClassNameW(hwnd, className, 256) && lstrcmpW(className, WEBM_WINDOW_CLASS) == 0) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0 && pid != GetCurrentProcessId()) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
    }
    return TRUE;
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
    IMFMediaEngine* pEngine; 
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
    MediaEngineNotifyC* pThis = (MediaEngineNotifyC*)thisPtr;
    if (event == MF_MEDIA_ENGINE_EVENT_ENDED && pThis->pEngine) {
        IMFMediaEngine_SetCurrentTime(pThis->pEngine, 0.0);
        IMFMediaEngine_Play(pThis->pEngine);
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

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    if (g_monitorCount >= OXOP_MAX_MONITORS) return FALSE;

    int x = lprcMonitor->left;
    int y = lprcMonitor->top;
    int w = lprcMonitor->right - lprcMonitor->left;
    int h = lprcMonitor->bottom - lprcMonitor->top;

    HWND hwnd = CreateWindowExW(0, WEBM_WINDOW_CLASS, L"Animated Wallpapers Reimagined", 
                               WS_POPUP | WS_VISIBLE, x, y, w, h, NULL, NULL, g_hInstance, NULL);

    HWND workerw = (HWND)dwData;
    if (workerw) SetParent(hwnd, workerw);

    MediaEngineNotifyC* pNotify = (MediaEngineNotifyC*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MediaEngineNotifyC));
    pNotify->Vtbl.lpVtbl = &g_NotifyVtbl;
    pNotify->m_cRef = 1;

    IMFAttributes* pAttributes = NULL;
    MFCreateAttributes(&pAttributes, 3);
    
    IMFAttributes_SetUnknown(pAttributes, &MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)pNotify);
    IMFAttributes_SetUINT64(pAttributes, &MF_MEDIA_ENGINE_PLAYBACK_HWND, (UINT64)hwnd);
    IMFAttributes_SetUINT32(pAttributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);

    IMFMediaEngine* pEngine = NULL;
    IMFMediaEngineClassFactory_CreateInstance(g_pFactory, 0, pAttributes, &pEngine);

    if (pEngine) {
        pNotify->pEngine = pEngine;

        BSTR bstrURL = SysAllocString(g_videoPath);
        IMFMediaEngine_SetSource(pEngine, bstrURL);
        IMFMediaEngine_Play(pEngine);
        SysFreeString(bstrURL);

        g_hwnds[g_monitorCount] = hwnd;
        g_engines[g_monitorCount] = pEngine;
        g_attributes[g_monitorCount] = pAttributes;
        g_monitorCount++;
    } else {
        IUnknown_Release(pAttributes);
        HeapFree(GetProcessHeap(), 0, pNotify);
        DestroyWindow(hwnd);
    }

    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    wchar_t lockPath[MAX_PATH];
    GetModuleFileNameW(NULL, lockPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(lockPath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    StringCchCatW(lockPath, MAX_PATH, L".LOCK");

    if (GetFileAttributesW(lockPath) != INVALID_FILE_ATTRIBUTES) {
        EnumWindows(KillOldInstancesProc, 0);
        Sleep(100);
    }

    if (!EnsureSingleInstance()) return 0;

    g_hInstance = hInstance;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    MFStartup(MF_VERSION, MFSTARTUP_FULL); 

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WEBM_WINDOW_CLASS;
    RegisterClassW(&wc);

    GetModuleFileNameW(NULL, g_videoPath, MAX_PATH);
    lastSlash = wcsrchr(g_videoPath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    StringCchCatW(g_videoPath, MAX_PATH, DEFAULT_VIDEO_NAME);

    CoCreateInstance(&CLSID_MFMediaEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IMFMediaEngineClassFactory, (void**)&g_pFactory);

    HWND workerw = GetWorkerW();

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)workerw);

    MSG msg = {0};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    for (int i = 0; i < g_monitorCount; i++) {
        if (g_engines[i]) IUnknown_Release(g_engines[i]);
        if (g_attributes[i]) IUnknown_Release(g_attributes[i]);
    }
    
    if (g_pFactory) IUnknown_Release(g_pFactory);

    MFShutdown();
    CoUninitialize();
    return 0;
}
