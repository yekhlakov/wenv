#include "Window.h"
#include "../Resource.h"

namespace Wenv::Display {


int CALLBACK EnumFontFamExProc (
    const LOGFONT *lpelfe,
    const TEXTMETRIC *lpntme,
    DWORD FontType,
    LPARAM lParam)
{
    auto *fontList = reinterpret_cast<std::vector<std::wstring>*>(lParam);

    if (!(lpntme->tmPitchAndFamily & TMPF_FIXED_PITCH))
    {
        if (fontList->empty () || fontList->back () != lpelfe->lfFaceName)
        {
            fontList->push_back (lpelfe->lfFaceName);
        }
    }

    return 1;
}

std::vector<std::wstring> GetMonospaceFonts (HDC hdc)
{
    std::vector<std::wstring> monospaceFonts;

    LOGFONT lf = { 0 };
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = L'\0';
    lf.lfPitchAndFamily = FIXED_PITCH;

    EnumFontFamiliesEx (hdc, &lf, (FONTENUMPROC) EnumFontFamExProc, (LPARAM) &monospaceFonts, 0);

    return monospaceFonts;
}


Window::Window(HINSTANCE hInstance, std::wstring title, std::wstring className)
    : hwnd(NULL), hdc(NULL), font_name(L"Consolas"), char_width(0), char_height(16), current_display(new Display()), hFont(NULL) {
    
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WENV));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WENV);
    wcex.lpszClassName = className.c_str();
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassExW(&wcex);

    hwnd = CreateWindowW(className.c_str(), title.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, this);

    if (hwnd) {
        hdc = GetDC(hwnd);
        monospace_fonts = GetMonospaceFonts(hdc);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

Window::~Window() {
    if (hFont) {
        DeleteObject(hFont);
    }
    if (hdc) {
        ReleaseDC(hwnd, hdc);
    }
    delete current_display;
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* pWindow = nullptr;

    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    } else {
        pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pWindow) {
        switch (message) {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hWnd, &ps);
                pWindow->draw(ps.hdc);
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_SIZE:
            pWindow->resize();
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void Window::set_font(std::wstring name) {
    if (hFont) {
        DeleteObject(hFont);
    }
    
    font_name = name;

    hFont = CreateFontW(
        16,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_MODERN,
        font_name.c_str()
    );

    if (hFont == NULL) return;

    SelectObject(hdc, hFont);
    
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    
    char_width = tm.tmAveCharWidth;
    char_height = tm.tmHeight;
}

void Window::resize() {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width_px = rect.right - rect.left;
    int height_px = rect.bottom - rect.top;

    if (char_width > 0 && char_height > 0) {
        size_t width_chars = static_cast<size_t>(width_px / char_width);
        size_t height_chars = static_cast<size_t>(height_px / char_height);
        current_display->resize(width_chars, height_chars);
    }
}

void Window::draw(HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    rect.top = 0;
    rect.bottom = 24;
    int size = 16;
    int cnt = 0;

    for (const std::wstring & fontName : monospace_fonts) {
        COLORREF colors[] = {
            RGB(0,0,0),
            RGB(255,0,0),
            RGB(0,255,0),
            RGB(255,255,0),
            RGB(0,0,255),
            RGB(255,0,255),
            RGB(0,255,255),
            RGB(255,255,255),
        };

        HFONT hFont = CreateFontW(
            size,
            0, 0, 0, 
            FW_NORMAL, 
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 
            OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, 
            FF_MODERN, 
            fontName.c_str());

        if (hFont) {
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            SetTextColor(hdc, colors[cnt & 7]);
            SetBkColor(hdc, colors[(cnt + 4) & 7]);
            SetBkMode(hdc, OPAQUE);

            DrawTextW(hdc, fontName.c_str(), -1, &rect, DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }

        cnt++;
        size += 4;
        rect.top += 2 + size;
        rect.bottom = rect.top + 4 + size;
    }
}

} // namespace Wenv::Display
