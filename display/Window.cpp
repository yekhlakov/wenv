
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
            {
                if (wParam != SIZE_MINIMIZED)
                {
                    pWindow->handle_resize (wParam, lParam);
                }
            }
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

void Window::handle_resize (WPARAM wParam, LPARAM lParam) {
	if (char_width <= 0 || char_height <= 0) {
		return;
	}

	RECT rect;
	GetClientRect(hwnd, &rect);
	int width_px = rect.right - rect.left;
	int height_px = rect.bottom - rect.top;

	int width_chars = width_px / char_width;
	int height_chars = height_px / char_height;

	if (width_chars > 0 && height_chars > 0) {
		// Use SWP_NOSIZE to prevent infinite loop if we are already at the correct size
		// and only resize if the dimensions actually change.
		// However, since we are snapping to char sizes, we should check if we need to resize.
		
		// We need to calculate the new window size based on client area.
		// For simplicity in this fix, we'll use SetWindowPos with the calculated client area dimensions.
		// To avoid infinite recursion in WM_SIZE -> SetWindowPos -> WM_SIZE, 
		// we should ensure we only call SetWindowPos if the new size is different from current window size.
		
		RECT window_rect;
		GetWindowRect(hwnd, &window_rect);
		int current_width = window_rect.right - window_rect.left;
		int current_height = window_rect.bottom - window_rect.top;

		// We need to account for non-client area (borders, title bar)
		// A better way is to use AdjustWindowRect to find the required window size for the target client area.
		RECT target_client_rect = { 0, 0, width_chars * char_width, height_chars * char_height };
		RECT target_window_rect = target_client_rect;
		AdjustWindowRect(&target_window_rect, WS_OVERLAPPEDWINDOW, FALSE);

		int target_width = target_window_rect.right - target_window_rect.left;
		int target_height = target_window_rect.bottom - target_window_rect.top;

		if (current_width != target_width || current_height != target_height) {
			SetWindowPos(hwnd, nullptr, 0, 0, target_width, target_height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		
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
