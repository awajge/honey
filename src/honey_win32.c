#ifndef UNICODE
#define UNICODE
#endif

#define _WIN32_WINNT 0x0A00 // target win10+

#include "honey_win32.h"
#include "internal.h"

#include <stdint.h>
#include <windows.h>

struct hny_icon_t {
    HICON hicon;
    size_t refc;
};

struct hny_cursor_t {
    HCURSOR hcursor;
    size_t refc;
};

struct hny_window_t {
    HWND hwnd;
    hny_window_callback_fn_t callback;
    
    hny_icon_t* ico;
    hny_cursor_t* cur;
    bool close;
};

// ===== internal

static inline void icon_destroy(hny_icon_t* icon) {
    if (!icon) return;

    DestroyIcon(icon->hicon);
    free(icon);
}

static inline void cursor_destroy(hny_cursor_t* cursor) {
    if (!cursor) return;

    DestroyCursor(cursor->hcursor);
    free(cursor);
}

static inline float dpi_to_scale(UINT dpi) {
    return (float) dpi / 96.0f;
}

static inline bool vec2_pos(hny_vec2_t vec2) {
    return vec2.x > 0 && vec2.y > 0;
}

LRESULT CALLBACK winproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    hny_window_t* window = (hny_window_t*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE: {
            if ((window = (hny_window_t*) ((CREATESTRUCTW*) lParam)->lpCreateParams)) {
                window->hwnd = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) window);
            }
            break;
        }

        case WM_CREATE: {
            if (window->ico) {
                SendMessageW(hwnd, WM_SETICON, ICON_BIG,   (LPARAM) window->ico->hicon);
                SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) window->ico->hicon);
            }
            return 0;
        }

        case WM_DESTROY: {
            window->close = true;
            return 0;
        }

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

        case WM_SYSCOMMAND: {
            switch (wParam & 0xFFF0) {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                case SC_KEYMENU:
                    return 0;
            }
            break;
        }
        
        /* ===== callback-proc messages ===== */ 
        case WM_CLOSE: {
            window->callback(window, &(hny_event_t) {
                .type = HNY_EVENT_CLOSE
            });
            return 0;
        }

        case WM_SIZE: {
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

        case WM_MOUSEMOVE: {
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

        case WM_INPUT: {
            RAWMOUSE m; RAWINPUT ri; UINT size = sizeof ri;
            if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER)) == (UINT) -1) break;
            if (ri.header.dwType != RIM_TYPEMOUSE || (m = ri.data.mouse).usFlags & MOUSE_MOVE_ABSOLUTE) break;
            
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
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ===== implementation
hny_icon_t* hny_icon_create(const wchar_t* path, hny_vec2_t size) {
    hny_icon_t* icon = malloc(sizeof(*icon));
    if (!icon) return NULL;
    *icon = (hny_icon_t) {
        .hicon = (HICON) LoadImageW(
            NULL,
            path,
            IMAGE_ICON,
            vec2_pos(size) ? size.x : 0,
            vec2_pos(size) ? size.y : 0,
            LR_LOADFROMFILE | (vec2_pos(size) ? 0 : LR_DEFAULTSIZE)
        ),
        .refc = 0
    };
    return icon->hicon ? icon : (free(icon), NULL);
}

hny_cursor_t* hny_cursor_create(const wchar_t* path, hny_vec2_t size) {
    hny_cursor_t* cursor = malloc(sizeof(*cursor));
    if (!cursor) return NULL;
    *cursor = (hny_cursor_t) {
        .hcursor = (HCURSOR) LoadImageW(
            NULL,
            path,
            IMAGE_CURSOR,
            vec2_pos(size) ? size.x : 0,
            vec2_pos(size) ? size.y : 0,
            LR_LOADFROMFILE | (vec2_pos(size) ? 0 : LR_DEFAULTSIZE)
        ),
        .refc = 0
    };
    return cursor->hcursor ? cursor : (free(cursor), NULL);
}

hny_window_t* hny_window_create(const wchar_t* name, hny_icon_t* ico, hny_cursor_t* cur, hny_window_callback_fn_t callback,
    hny_vec2_t off, hny_vec2_t size) {
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
    }

    if (!name) return NULL;
    if (!vec2_pos(size)) return NULL;

    hny_window_t* window = malloc(sizeof(*window));
    if (!window) return NULL;
    *window = (hny_window_t) {
        .callback = callback,
        .ico = ico,
        .cur = cur,
        .close = false
    };
    
    HWND hwnd = CreateWindowExW(
        0,
        MAKEINTATOM(wcatom),
        name,
        WS_OVERLAPPEDWINDOW,
        vec2_pos(off) ? off.x : 0,
        vec2_pos(off) ? off.y : 0,
        size.x,
        size.y,
        NULL,
        NULL,
        GetModuleHandleW(NULL),
        window
    ); if (!hwnd) return free(window), NULL;

    if (ico) ico->refc++;
    if (cur) cur->refc++;

    return window; 
}

void hny_window_destroy(hny_window_t* window) {
    if (!window) return;

    if (window->ico && !--window->ico->refc) icon_destroy(window->ico);
    if (window->cur && !--window->cur->refc) cursor_destroy(window->cur);
    window->ico = NULL, window->cur = NULL;

    DestroyWindow(window->hwnd);
    free(window);
}

void hny_window_hide(hny_window_t* window) {
    if (window) ShowWindow(window->hwnd, SW_HIDE);
}

void hny_window_show(hny_window_t* window) {
    if (window) ShowWindow(window->hwnd, SW_SHOW);
}

void hny_window_minimize(hny_window_t* window) {
    if (window) ShowWindow(window->hwnd, SW_MINIMIZE);
}

void hny_window_maximize(hny_window_t* window) {
    if (window) ShowWindow(window->hwnd, SW_MAXIMIZE);
}

void hny_window_restore(hny_window_t* window) {
    if (window) ShowWindow(window->hwnd, SW_RESTORE);
}

bool hny_window_should_close(const hny_window_t* window) {
    return window ? window->close : false;
}

void hny_window_set_close(hny_window_t* window) {
    if (window) window->close = true;
}

float hny_window_dpi_scale(hny_window_t* window) {
    return window && window->hwnd ? dpi_to_scale(GetDpiForWindow(window->hwnd)) : 0.0f;
}

void hny_dispatch_events(void) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}