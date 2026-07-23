
#include "Display.h"
#include "Palette.h"
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


Window::Window(HINSTANCE hInstance, std::wstring title, std::wstring className):
	hwnd {NULL}, 
	hdc {NULL}, 
	font_name {L"Consolas"},
	char_width {0},
	char_height {32},
	current_display {new Display()},
	current_palette {new Palette()},
	hFont {NULL}
{
    
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

	    set_font (font_name);

        monospace_fonts = GetMonospaceFonts(hdc);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

Window::~Window()
{
    if (hFont)
    {
        DeleteObject (hFont);
    }

    if (hdc)
    {
        ReleaseDC (hwnd, hdc);
    }

    delete current_display;
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow = nullptr;

    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    }
    else
    {
        pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (!pWindow)
    {
        return DefWindowProc (hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            pWindow->draw(ps.hdc);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_SIZING:
        pWindow->handle_resizing (wParam, lParam);
        break;

    case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                pWindow->handle_resize (wParam, lParam);
            }
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_CLOSE:
        DestroyWindow (hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SYSKEYDOWN:

        if (wParam == VK_F10)
        {
            // Shortcut to kill the application
            DestroyWindow (hWnd);
        }
        else if (wParam == VK_SHIFT)
        {
            // Alt+Shift switches keyboard layout
            ActivateKeyboardLayout ((HKL) HKL_NEXT, 0);
        }

        return 0; // Don't propagate syskeys to default window proc

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;    
}

void Window::set_font(std::wstring name) {
    if (hFont) {
        DeleteObject(hFont);
    }
    
    font_name = name;

    hFont = CreateFontW(
        char_height,
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

void Window::handle_resizing (WPARAM wParam, LPARAM lParam)
{
    if (char_width <= 0 || char_height <= 0)
    {
        return;
    }

    RECT client_rect, window_rect;
    GetClientRect (hwnd, &client_rect);
    GetWindowRect (hwnd, &window_rect);

    // Compute size difference between window client area and its gross area
    int dw = (window_rect.right - window_rect.left) - (client_rect.right - client_rect.left);
    int dh = (window_rect.bottom - window_rect.top) - (client_rect.bottom - client_rect.top);

    // Supposed new window size
    RECT *rect = (RECT *) lParam;

    // Sizes in pixels
    int width_px = rect->right - rect->left;
    int height_px = rect->bottom - rect->top;

    // Sizes in characters
    int width_chars = (width_px - dw) / char_width;
    int height_chars = (height_px - dh) / char_height;

    // Target width in pixels (snapped to character size)
    int target_width = width_chars * char_width + dw;
    int target_height = height_chars * char_height + dh;

    // Now adjust the dragged edge position
    if (wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOMRIGHT || wParam == WMSZ_TOPRIGHT)
    {
        // Adjust right edge
        rect->right = rect->left + target_width;
    }

    if (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_TOPLEFT)
    {
        // Adjust left edge
        rect->left = rect->right - target_width;
    }

    if (wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT)
    {
        // Adjust bottom edge
        rect->bottom = rect->top + target_height;
    }

    if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
    {
        // Adjust top edge
        rect->top = rect->bottom - target_height;
    }
}

void Window::handle_resize (WPARAM wParam, LPARAM lParam)
{
    if (char_width <= 0 || char_height <= 0)
    {
        return;
    }

    int width_px = LOWORD (lParam);
    int height_px = HIWORD (lParam);

	int width_chars = width_px / char_width;
	int height_chars = height_px / char_height;

	if (width_chars > 0 && height_chars > 0)
    {
		current_display->resize(static_cast<size_t>(width_chars), static_cast<size_t>(height_chars));
	}
}

void Window::draw (HDC hdc) {
    HFONT hOldFont = (HFONT) SelectObject (hdc, hFont);

    SetBkMode (hdc, OPAQUE);
    int current_bg = -1, current_fg = -1;
    std::wstring current_ln = L"";

    // Helper to draw the batch and reset
    auto flush_batch = [&] (int x_start, int y_start, std::wstring &str) {
        if (!str.empty ()) {
            TextOutW (hdc, x_start * char_width, y_start * char_height, str.c_str (), static_cast<int>(str.length ()));
            str.clear ();
        }
    };

    for (size_t y = 0; y < current_display->data.size (); ++y) {
        // Reset row tracking for every new line
        int row_start_x = 0;

        for (size_t x = 0; x < current_display->data[y].size (); ++x) {

            const Character &ch = current_display->data[y][x];
            int incoming_fg = ch.fg_color;
            int incoming_bg = ch.bg_color;

            if (ch.palette_color != -1) {
                auto entry = current_palette->get_entry (ch.palette_color);
                incoming_fg = entry.foreground_color;
                incoming_bg = entry.background_color;
            }

            // If color changes
            if (incoming_fg != current_fg || incoming_bg != current_bg) {
                // 1. Flush the previous color block
                flush_batch (row_start_x, static_cast<int>(y), current_ln);

                // 2. Apply new colors
                current_fg = incoming_fg;
                current_bg = incoming_bg;
                SetTextColor (hdc, current_fg);
                SetBkColor (hdc, current_bg);

                // 3. Start new block at current X
                row_start_x = static_cast<int>(x);
            }

            current_ln += ch.value;
        }

        // End of row: Flush whatever is left in the current line
        flush_batch (row_start_x, static_cast<int>(y), current_ln);

        // Reset color state to force a refresh on the next row 
        // (prevents color bleeding if rows have different starting colors)
        current_fg = -1;
        current_bg = -1;
    }

    SelectObject (hdc, hOldFont);
}


/*
RECT rect;
GetClientRect(hwnd, &rect);
rect.top = 0;
rect.bottom = rect.top + 24;
int size = 24;
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

    HFONT hFont_loop = CreateFontW(
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

    if (hFont_loop) {
        HFONT hOldFont_loop = (HFONT)SelectObject(hdc, hFont_loop);

        SetTextColor(hdc, colors[cnt & 7]);
        SetBkColor(hdc, colors[(cnt + 4) & 7]);
        SetBkMode(hdc, OPAQUE);

        DrawTextW(hdc, fontName.c_str(), -1, &rect, DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont_loop);
        DeleteObject(hFont_loop);
    }

    cnt++;
    rect.top += 2 + size;
    rect.bottom = rect.top + 4 + size;
}
*/

} // namespace Wenv::Display
