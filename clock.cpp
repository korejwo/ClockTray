#include <windows.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TIMER 1
extern "C" int _fltused = 0;
extern "C" void* memset(void* dst, int val, size_t len)
{
    unsigned char* p = (unsigned char*)dst;
    while (len--) *p++ = (unsigned char)val;
    return dst;
}

HWND hWnd;
NOTIFYICONDATA nid;
HMENU hMenu;
HFONT hFont = NULL;

COLORREF currentTextColor = RGB(0, 0, 0);
COLORREF targetTextColor = RGB(0, 0, 0);

COLORREF GetBackgroundPixelColorUnderWindow(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);

    int x = (rect.left + rect.right) / 2;
    int y = (rect.top + rect.bottom) / 2;

    HDC hScreenDC = GetDC(NULL);
    COLORREF color = GetPixel(hScreenDC, x, y);
    ReleaseDC(NULL, hScreenDC);

    return color;
}

COLORREF ChooseTextColorBasedOnBackground(COLORREF bg)
{
    int r = GetRValue(bg);
    int g = GetGValue(bg);
    int b = GetBValue(bg);
    int luma = (299 * r + 587 * g + 114 * b) / 1000;

    return (luma > 128) ? RGB(0, 0, 0) : RGB(255, 255, 255);
}

COLORREF InterpolateColor(COLORREF from, COLORREF to, double factor)
{
    int r = (int)(GetRValue(from) + factor * (GetRValue(to) - GetRValue(from)));
    int g = (int)(GetGValue(from) + factor * (GetGValue(to) - GetGValue(from)));
    int b = (int)(GetBValue(from) + factor * (GetBValue(to) - GetBValue(from)));
    return RGB(r, g, b);
}

void GetCurrentTimeStr(wchar_t* buffer)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    wsprintfW(buffer, L"%02d:%02d:%02d", t.wHour, t.wMinute, t.wSecond);
}

void UpdateTime()
{
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void SaveWindowPosition(int x, int y)
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ClockTrayApp", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, L"PosX", 0, REG_DWORD, (const BYTE*)&x, sizeof(DWORD));
        RegSetValueExW(hKey, L"PosY", 0, REG_DWORD, (const BYTE*)&y, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

BOOL LoadWindowPosition(int* x, int* y)
{
    HKEY hKey;
    DWORD dataSize = sizeof(DWORD);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ClockTrayApp", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD valX, valY;
        if (RegQueryValueExW(hKey, L"PosX", NULL, NULL, (LPBYTE)&valX, &dataSize) == ERROR_SUCCESS &&
            RegQueryValueExW(hKey, L"PosY", NULL, NULL, (LPBYTE)&valY, &dataSize) == ERROR_SUCCESS)
        {
            *x = (int)valX;
            *y = (int)valY;
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hwnd, ID_TIMER, 100, NULL); // co 100 ms dla p³ynnej zmiany koloru
        hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, L"Segoe UI");
        break;

    case WM_TIMER:
    {
        COLORREF bg = GetBackgroundPixelColorUnderWindow(hwnd);
        COLORREF desiredColor = ChooseTextColorBasedOnBackground(bg);

        if (desiredColor != targetTextColor) {
            targetTextColor = desiredColor;
        }

        currentTextColor = InterpolateColor(currentTextColor, targetTextColor, 0.2);
        UpdateTime();
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 255)); // kolor t³a - przezroczysty
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, currentTextColor);

        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        wchar_t timeStr[16];
        GetCurrentTimeStr(timeStr);
        DrawTextW(hdc, timeStr, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        DeleteObject(hFont);
        KillTimer(hwnd, ID_TIMER);
        RECT rect;
        GetWindowRect(hwnd, &rect);
        SaveWindowPosition(rect.left, rect.top);
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WINAPI RawEntry()
{
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ClockTrayApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    int x = 100, y = 100, width = 200, height = 100;
    LoadWindowPosition(&x, &y);
    hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Clock", WS_POPUP,
        x, y, width, height, NULL, NULL, wc.hInstance, NULL);

    SetLayeredWindowAttributes(hWnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Ikona w trayu
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(101));
    lstrcpyW(nid.szTip, L"Zegar");
    Shell_NotifyIconW(NIM_ADD, &nid);

    // Menu kontekstowe
    hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Zamknij");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyMenu(hMenu);
    ExitProcess(0);
}
