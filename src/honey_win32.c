#ifndef UNICODE
#define UNICODE
#endif

#define _WIN32_WINNT 0x0A00

#include "honey_win32.h"
#include "internal.h"

#include <stddef.h>
#include <windows.h>

struct hny_icon_t {
    HICON hicon;
};

struct hny_cursor_t {
    HCURSOR hcursor;
};

struct hny_window_t {
    HWND hwnd;
    hny_window_callback_fn_t callback;
    hny_cursor_t* cur;
};

// ===== internal
static inline wchar* char_to_wchar(const char* str) {
    if (!str) return NULL;

    int num = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
    if (!num) return NULL;

    wchar* wstr = malloc(num * sizeof *wstr);
    if (!wstr) return NULL;

    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wstr, num) ? wstr : (free(wstr), NULL);
}

static inline float dpi_to_scale(UINT dpi) {
    return (float) dpi / 96.0f;
}

static inline bool vec2_ge(hny_vec2_t vec2, int i) {
    return vec2.x >= i && vec2.y >= i;
}

static LRESULT CALLBACK winproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    hny_window_t* window = (hny_window_t*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    // === default window behavior
    if (window) switch (msg) {
        case WM_NCDESTROY: {
            window->hwnd = NULL;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            return 0;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT)  {
                SetCursor(window->cur ? window->cur->hcursor : LoadCursorW(NULL, IDC_ARROW));
                return TRUE;
            }
            break;
        }
        
        case WM_PAINT: {
            ValidateRect(hwnd, NULL);
            return 0;
        }

        case WM_ERASEBKGND: {
            return 1;
        }
    }

    if (window && window->callback) switch (msg) {
        case WM_DESTROY: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_DESTROY
            });
            return 0;
        }

        case WM_CLOSE: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_CLOSE
            });
            return 0;
        }

        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) return 0;
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_POSTRESIZE,
                .u.vec2 = {
                    .x = LOWORD(lParam),
                    .y = HIWORD(lParam)
                }
            });
            return 0;
        }

        case WM_SETFOCUS: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_FOCUS,
            });
            return 0;
        }

        case WM_KILLFOCUS: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_UNFOCUS,
            });
            return 0;
        }

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            int scancode = HIWORD(lParam) & (KF_EXTENDED | 0xFF);
            if (!(scancode & 0xFF))
                scancode |= (int) MapVirtualKeyW((UINT) wParam, MAPVK_VK_TO_VSC);

            if (scancode == 0x54)  scancode = 0x0137; // alt+prtsc = prtsc
            if (scancode == 0x146) scancode = 0x0045; // legacy pause 
            if (scancode == 0x136) scancode = 0x0036; // cjk ime extended rshift bit

            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_KEY,
                .u.key_info = {
                    .key = (hny_key_t) scancode,
                    .action = HIWORD(lParam) & KF_UP
                        ? HNY_INPUT_ACTION_RELEASE
                        : HIWORD(lParam) & KF_REPEAT ? HNY_INPUT_ACTION_REPEAT : HNY_INPUT_ACTION_PRESS
                }
            });
            return 0;
        }

        case WM_MOUSEMOVE: { // cursor move, pixels
            POINTS p = MAKEPOINTS(lParam);
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_MOUSE_MOVE,
                .u.vec2 = {
                    .x = p.x,
                    .y = p.y
                }
            });
            return 0;
        }

        case WM_INPUT: { // mouse hardware move, not pixels
            RAWMOUSE m; RAWINPUT ri; UINT size = sizeof ri;
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER)) == (UINT) -1) break;
            if (ri.header.dwType != RIM_TYPEMOUSE || (m = ri.data.mouse).usFlags & MOUSE_MOVE_ABSOLUTE) break;
            if (!m.lLastX && !m.lLastY) break;  
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_MOUSE_DELTA,
                .u.vec2 = {
                    .x = m.lLastX,
                    .y = m.lLastY
                }
            });
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP: {
            hny_mouse_button_t button = HNY_MOUSE_BUTTON_LEFT;

            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    button = HNY_MOUSE_BUTTON_RIGHT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    button = HNY_MOUSE_BUTTON_MIDDLE;
                    break;
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                    button = GET_XBUTTON_WPARAM(wParam) == XBUTTON1
                        ? HNY_MOUSE_BUTTON_X1
                        : HNY_MOUSE_BUTTON_X2;
                    break;
            }

            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_MOUSE_BUTTON,
                .u.button_info = {
                    .button = button,
                    .action = (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN)
                        ? HNY_INPUT_ACTION_PRESS
                        : HNY_INPUT_ACTION_RELEASE
                }
            });
            return button == HNY_MOUSE_BUTTON_X1 || button == HNY_MOUSE_BUTTON_X2 ? TRUE : 0;
        }

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
            const float delta = GET_WHEEL_DELTA_WPARAM(wParam) / (float) WHEEL_DELTA;
            const bool vert = msg == WM_MOUSEWHEEL;
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_MOUSE_WHEEL,
                .u.wheel_info = {
                    .x = vert ? 0.0f : delta,
                    .y = vert ? delta : 0.0f
                }
            });
            return 0;
        }

        case WM_DPICHANGED: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_DPICHANGE,
                .u.dpi_scale = dpi_to_scale(LOWORD(wParam))
            });
            return 0;
        }
    
        case WM_SIZING: {
            RECT f = {0}, *r = (RECT*) lParam;
            AdjustWindowRectExForDpi(
                &f,
                (DWORD) GetWindowLongPtrW(hwnd, GWL_STYLE), FALSE,
                (DWORD) GetWindowLongPtrW(hwnd, GWL_EXSTYLE), GetDpiForWindow(hwnd)
            );

            hny_event_t e = {
                .type = HNY_EVENT_USERRESIZE,
                .u.rect = {
                    .tl = { .x = r->left   - f.left,  .y = r->top    - f.top },
                    .br = { .x = r->right  - f.right, .y = r->bottom - f.bottom }
                }
            };
            window->callback(window, &e);

            r->left   = e.u.rect.tl.x + f.left;
            r->top    = e.u.rect.tl.y + f.top;
            r->right  = e.u.rect.br.x + f.right;
            r->bottom = e.u.rect.br.y + f.bottom;
            return TRUE;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ===== implementation

// === icon
hny_icon_t* hny_icon_create(const char* path, hny_vec2_t size) {
    hny_icon_t* icon = malloc(sizeof(*icon));
    if (!icon) return NULL;

    wchar* wpath = char_to_wchar(path);
    if (path && !wpath) return free(icon), NULL;
    if (!path) return *icon = (hny_icon_t) { .hicon = NULL }, icon;

    *icon = (hny_icon_t) {
        .hicon = (HICON) LoadImageW(
            NULL,
            wpath,
            IMAGE_ICON,
            vec2_ge(size, 1) ? size.x : 0,
            vec2_ge(size, 1) ? size.y : 0,
            LR_LOADFROMFILE | (vec2_ge(size, 1) ? 0 : LR_DEFAULTSIZE)
        ),
    };
    free(wpath);
    return icon->hicon ? icon : (free(icon), NULL);
}

void hny_window_set_icon(hny_window_t* window, const hny_icon_t* icon, hny_icon_type_t type) {
    if (!hny_window_valid(window)) return;
    switch (type) {
        case HNY_ICON_SMALL: SendMessageW(window->hwnd, WM_SETICON, ICON_SMALL, (LPARAM) (icon ? icon->hicon : NULL)); break;
        case HNY_ICON_BIG:   SendMessageW(window->hwnd, WM_SETICON, ICON_BIG,   (LPARAM) (icon ? icon->hicon : NULL)); break;
    }
}

void hny_icon_free(hny_icon_t* icon) {
    if (icon) DestroyIcon(icon->hicon), free(icon);
}

// === cursor
hny_cursor_t* hny_cursor_create(const char* path, hny_vec2_t size) {
    hny_cursor_t* cursor = malloc(sizeof(*cursor));
    if (!cursor) return NULL;

    wchar* wpath = char_to_wchar(path);
    if (path && !wpath) return free(cursor), NULL;
    if (!path) return *cursor = (hny_cursor_t) { .hcursor = NULL }, cursor;

    *cursor = (hny_cursor_t) {
        .hcursor = (HCURSOR) LoadImageW(
            NULL,
            wpath,
            IMAGE_CURSOR,
            vec2_ge(size, 1) ? size.x : 0,
            vec2_ge(size, 1) ? size.y : 0,
            LR_LOADFROMFILE | (vec2_ge(size, 1) ? 0 : LR_DEFAULTSIZE)
        )
    };
    free(wpath);
    return cursor->hcursor ? cursor : (free(cursor), NULL);
}

void hny_window_set_cursor(hny_window_t* window, hny_cursor_t* cur) {
    if (!hny_window_valid(window)) return;
    
    window->cur = cur;
    POINT p; if (!GetCursorPos(&p) || WindowFromPoint(p) != window->hwnd) return;
    SendMessageW( // send cursor event w/ hit test results
        window->hwnd,
        WM_SETCURSOR,
        (WPARAM) window->hwnd,
        MAKELPARAM(SendMessageW(window->hwnd, WM_NCHITTEST, 0, MAKELPARAM(p.x, p.y)), WM_MOUSEMOVE)
    );
}

void hny_cursor_free(hny_cursor_t* cur) {
    if (cur) DestroyCursor(cur->hcursor), free(cur);
}

// === window
hny_window_t* hny_window_create(hny_vec2_t off, hny_vec2_t size) {
    static ATOM wcatom;
    if (!wcatom) {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        RAWINPUTDEVICE rid = {
            .usUsagePage = 0x01,
            .usUsage     = 0x02,
            .dwFlags     = 0,
            .hwndTarget  = NULL
        };
        RegisterRawInputDevices(&rid, 1, sizeof rid);
    
        WNDCLASSEXW wc = {
            .lpszClassName  = L"hny_window_class",
            .lpfnWndProc    = winproc,
            .hInstance      = GetModuleHandleW(NULL),
            .hCursor        = NULL,
            .hIcon          = NULL,
            .hbrBackground  = NULL,
            .cbSize         = sizeof wc
        };
        
        wcatom = RegisterClassExW(&wc);
        if (!wcatom) return NULL;
    }
    
    hny_window_t* window = calloc(1, sizeof(*window));
    if (!window) return NULL;

    DWORD exstyle = 0, style = WS_OVERLAPPEDWINDOW;

    RECT rect = {.left= 0, .top = 0, .right = size.x, .bottom = size.y};
    UINT dpi = GetDpiForSystem();
    AdjustWindowRectExForDpi(&rect, style, FALSE, exstyle, dpi);

    HWND hwnd = CreateWindowExW(
        exstyle,
        MAKEINTATOM(wcatom),
        NULL,
        style,
        vec2_ge(off, 0) ? off.x + rect.left : CW_USEDEFAULT,
        vec2_ge(off, 0) ? off.y + rect.top  : CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL,
        GetModuleHandleW(NULL),
        NULL
    ); if (!hwnd) return free(window), NULL;

    return window->hwnd = hwnd, SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, (LONG_PTR) window), window; 
}

void hny_window_set_title(hny_window_t* window, const char* name) {
    if (!hny_window_valid(window)) return;

    wchar* wname = char_to_wchar(name);
    if (name && !wname) return;

    SetWindowTextW(window->hwnd, wname);
    free(wname);
}

void hny_window_hide(hny_window_t* window) {
    if (hny_window_valid(window)) ShowWindow(window->hwnd, SW_HIDE);
}

void hny_window_show(hny_window_t* window) {
    if (hny_window_valid(window)) ShowWindow(window->hwnd, SW_SHOW);
}

void hny_window_set_visible(hny_window_t* window, bool vis) {
    if (hny_window_valid(window)) ShowWindow(window->hwnd, vis ? SW_SHOW : SW_HIDE);
}

bool hny_window_visible(const hny_window_t* window) {
    return hny_window_valid(window) && IsWindowVisible(window->hwnd);
}

void hny_window_set_state(hny_window_t* window, hny_window_state_t state) {
    if (!hny_window_valid(window)) return;
    switch (state) {
        case HNY_WINDOW_STATE_MAXIMIZE: ShowWindow(window->hwnd, SW_MAXIMIZE); break;
        case HNY_WINDOW_STATE_MINIMIZE: ShowWindow(window->hwnd, SW_MINIMIZE); break;
        case HNY_WINDOW_STATE_RESTORE:  ShowWindow(window->hwnd, SW_RESTORE);  break;
    }
}

float hny_window_dpi_scale(const hny_window_t* window) {
    return hny_window_valid(window) ? dpi_to_scale(GetDpiForWindow(window->hwnd)) : 0.0f;
}

bool hny_window_rect(const hny_window_t* window, hny_rect_t* out) {
    if (!hny_window_valid(window) || !out) return false;

    RECT r; 
    if (!GetClientRect(window->hwnd, &r)) return false;

    POINT tl = { .x = r.left,  .y = r.top };
    POINT br = { .x = r.right, .y = r.bottom };
    if (!ClientToScreen(window->hwnd, &tl) || !ClientToScreen(window->hwnd, &br)) return false;
    
    return *out = (hny_rect_t) {
        .tl = { .x = tl.x, .y = tl.y },
        .br = { .x = br.x, .y = br.y }
    }, true;
}

void hny_window_set_rect(hny_window_t* window, hny_rect_t rect) {
    if (!hny_window_valid(window)) return;

    RECT r = { .left = rect.tl.x, .top = rect.tl.y, .right = rect.br.x, .bottom = rect.br.y };
    AdjustWindowRectExForDpi(
        &r,
        (DWORD) GetWindowLongPtrW(window->hwnd, GWL_STYLE), FALSE,
        (DWORD) GetWindowLongPtrW(window->hwnd, GWL_EXSTYLE), GetDpiForWindow(window->hwnd)
    );

    SetWindowPos(window->hwnd, NULL, r.left, r.top, r.left - r.right, r.top - r.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
}

bool hny_window_destroy(hny_window_t* window) {
    if (hny_window_valid(window)) DestroyWindow(window->hwnd);
    return !hny_window_valid(window);
}

bool hny_window_free(hny_window_t* window) {
    return hny_window_destroy(window) ? free(window), true : false;
}

bool hny_window_valid(const hny_window_t* window) {
    return window && window->hwnd;
}

void hny_window_set_callback(hny_window_t* window, hny_window_callback_fn_t callback) {
    if (hny_window_valid(window)) window->callback = callback;
}

bool hny_cursor_pos(hny_vec2_t* out) {
    POINT p;
    if (!out || !GetCursorPos(&p)) return false;
    return *out = (hny_vec2_t) {
        .x = p.x,
        .y = p.y
    }, true;
}

void hny_dispatch_events(void) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}