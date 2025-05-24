#include <windows.h>
#include <time.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TIMER 1

HINSTANCE hInst;
HWND hWnd;
NOTIFYICONDATA nid;
HMENU hMenu;

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
    double luma = 0.299 * r + 0.587 * g + 0.114 * b;

    return (luma > 128) ? RGB(0, 0, 0) : RGB(255, 255, 255);
}

COLORREF InterpolateColor(COLORREF from, COLORREF to, double factor)
{
    int r = (int)(GetRValue(from) + factor * (GetRValue(to) - GetRValue(from)));
    int g = (int)(GetGValue(from) + factor * (GetGValue(to) - GetGValue(from)));
    int b = (int)(GetBValue(from) + factor * (GetBValue(to) - GetBValue(from)));
    return RGB(r, g, b);
}

void GetCurrentTimeStr(wchar_t* buffer, int size)
{
    time_t now = time(NULL);
    struct tm localTime;
    localtime_s(&localTime, &now);
    wcsftime(buffer, size, L"%H:%M:%S", &localTime);
}

void UpdateTime()
{
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetTimer(hwnd, ID_TIMER, 100, NULL); // co 100 ms dla p³ynnej zmiany koloru
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

        HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            VARIABLE_PITCH, L"Segoe UI");

        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        wchar_t timeStr[16];
        GetCurrentTimeStr(timeStr, 16);
        DrawTextW(hdc, timeStr, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
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
        KillTimer(hwnd, ID_TIMER);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ClockTrayApp";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    int width = 200, height = 100;
    hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Clock", WS_POPUP,
        100, 100, width, height, NULL, NULL, hInstance, NULL);

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
    nid.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(101));
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
    return 0;
}
