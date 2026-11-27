#ifndef HONEY_WIN32_H
#define HONEY_WIN32_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hny_vec2_t {
    int x, y;
} hny_vec2_t;

typedef struct hny_rect_t {
    hny_vec2_t tl, br;
} hny_rect_t;

typedef enum hny_window_state_t {
    HNY_WINDOW_STATE_MAXIMIZE,
    HNY_WINDOW_STATE_MINIMIZE,
    HNY_WINDOW_STATE_RESTORE
} hny_window_state_t;

typedef enum hny_icon_type_t {
    HNY_ICON_SMALL,
    HNY_ICON_BIG
} hny_icon_type_t;

typedef enum hny_input_action_t {
    HNY_INPUT_ACTION_PRESS,
    HNY_INPUT_ACTION_RELEASE,
    HNY_INPUT_ACTION_REPEAT
} hny_input_action_t;

typedef enum hny_key_t {    
    HNY_KEY_A = 0x001E, HNY_KEY_B = 0x0030, HNY_KEY_C = 0x002E, HNY_KEY_D = 0x0020,
    HNY_KEY_E = 0x0012, HNY_KEY_F = 0x0021, HNY_KEY_G = 0x0022, HNY_KEY_H = 0x0023,
    HNY_KEY_I = 0x0017, HNY_KEY_J = 0x0024, HNY_KEY_K = 0x0025, HNY_KEY_L = 0x0026,
    HNY_KEY_M = 0x0032, HNY_KEY_N = 0x0031, HNY_KEY_O = 0x0018, HNY_KEY_P = 0x0019,
    HNY_KEY_Q = 0x0010, HNY_KEY_R = 0x0013, HNY_KEY_S = 0x001F, HNY_KEY_T = 0x0014,
    HNY_KEY_U = 0x0016, HNY_KEY_V = 0x002F, HNY_KEY_W = 0x0011, HNY_KEY_X = 0x002D,
    HNY_KEY_Y = 0x0015, HNY_KEY_Z = 0x002C,

    HNY_KEY_1 = 0x0002, HNY_KEY_2 = 0x0003, HNY_KEY_3 = 0x0004, HNY_KEY_4 = 0x0005,
    HNY_KEY_5 = 0x0006, HNY_KEY_6 = 0x0007, HNY_KEY_7 = 0x0008, HNY_KEY_8 = 0x0009,
    HNY_KEY_9 = 0x000A, HNY_KEY_0 = 0x000B,

    HNY_KEY_SPACE         = 0x0039, HNY_KEY_DASH          = 0x000C, HNY_KEY_EQUAL         = 0x000D,
    HNY_KEY_LEFT_BRACKET  = 0x001A, HNY_KEY_RIGHT_BRACKET = 0x001B, HNY_KEY_BACKSLASH     = 0x002B,
    HNY_KEY_SEMICOLON     = 0x0027, HNY_KEY_APOSTROPHE    = 0x0028, HNY_KEY_GRAVE         = 0x0029,
    HNY_KEY_COMMA         = 0x0033, HNY_KEY_PERIOD        = 0x0034, HNY_KEY_SLASH         = 0x0035,

    HNY_KEY_ENTER       = 0x001C, HNY_KEY_ESCAPE      = 0x0001, HNY_KEY_BACKSPACE   = 0x000E,
    HNY_KEY_TAB         = 0x000F, HNY_KEY_CAPSLOCK    = 0x003A, HNY_KEY_PRTSC       = 0x0137,
    HNY_KEY_SCRLCK      = 0x0046, HNY_KEY_PAUSE       = 0x0045, HNY_KEY_INSERT      = 0x0152,
    HNY_KEY_HOME        = 0x0147, HNY_KEY_PAGEUP      = 0x0149, HNY_KEY_DELETE      = 0x0153,
    HNY_KEY_END         = 0x014F, HNY_KEY_WORLD       = 0x0056, HNY_KEY_MENU        = 0x015D,
    HNY_KEY_PAGEDOWN    = 0x0151, HNY_KEY_LEFT_ARROW  = 0x014B, HNY_KEY_RIGHT_ARROW = 0x014D,
    HNY_KEY_UP_ARROW    = 0x0148, HNY_KEY_DOWN_ARROW  = 0x0150,

    HNY_KEY_LEFT_CTRL   = 0x001D, HNY_KEY_RIGHT_CTRL  = 0x011D, HNY_KEY_LEFT_SHIFT  = 0x002A,
    HNY_KEY_RIGHT_SHIFT = 0x0036, HNY_KEY_LEFT_ALT    = 0x0038, HNY_KEY_RIGHT_ALT   = 0x0138,
    HNY_KEY_LEFT_META   = 0x015B, HNY_KEY_RIGHT_META  = 0x015C,

    HNY_KEY_F1  = 0x003B, HNY_KEY_F2  = 0x003C, HNY_KEY_F3  = 0x003D, HNY_KEY_F4  = 0x003E,
    HNY_KEY_F5  = 0x003F, HNY_KEY_F6  = 0x0040, HNY_KEY_F7  = 0x0041, HNY_KEY_F8  = 0x0042,
    HNY_KEY_F9  = 0x0043, HNY_KEY_F10 = 0x0044, HNY_KEY_F11 = 0x0057, HNY_KEY_F12 = 0x0058,
    HNY_KEY_F13 = 0x0064, HNY_KEY_F14 = 0x0065, HNY_KEY_F15 = 0x0066, HNY_KEY_F16 = 0x0067,
    HNY_KEY_F17 = 0x0068, HNY_KEY_F18 = 0x0069, HNY_KEY_F19 = 0x006A, HNY_KEY_F20 = 0x006B,
    HNY_KEY_F21 = 0x006C, HNY_KEY_F22 = 0x006D, HNY_KEY_F23 = 0x006E, HNY_KEY_F24 = 0x0076,

    HNY_KEY_INTL1 = 0x0073, HNY_KEY_INTL2 = 0x0070, HNY_KEY_INTL3 = 0x007D, HNY_KEY_INTL4 = 0x0079,
    HNY_KEY_INTL5 = 0x007B, HNY_KEY_INTL6 = 0x005C,

    HNY_KEY_LANG3 = 0x0078, HNY_KEY_LANG4 = 0x0077, HNY_KEY_LANG5 = 0x0076,

    HNY_KEY_KP_NUMLOCK  = 0x0145, HNY_KEY_KP_ENTER    = 0x011C, HNY_KEY_KP_COMMA    = 0x007E,
    HNY_KEY_KP_EQUAL    = 0x0059, HNY_KEY_KP_DECIMAL  = 0x0053, HNY_KEY_KP_DIVIDE   = 0x0135,
    HNY_KEY_KP_MULTIPLY = 0x0137, HNY_KEY_KP_SUBTRACT = 0x014A, HNY_KEY_KP_ADD      = 0x014E,

    HNY_KEY_KP_1 = 0x004F, HNY_KEY_KP_2 = 0x0050, HNY_KEY_KP_3 = 0x0051, HNY_KEY_KP_4 = 0x004B,
    HNY_KEY_KP_5 = 0x004C, HNY_KEY_KP_6 = 0x004D, HNY_KEY_KP_7 = 0x0047, HNY_KEY_KP_8 = 0x0048,
    HNY_KEY_KP_9 = 0x0049, HNY_KEY_KP_0 = 0x0052
} hny_key_t;

typedef enum hny_mouse_button_t {
    HNY_MOUSE_BUTTON_LEFT,
    HNY_MOUSE_BUTTON_RIGHT,
    HNY_MOUSE_BUTTON_MIDDLE,
    HNY_MOUSE_BUTTON_X1,
    HNY_MOUSE_BUTTON_X2
} hny_mouse_button_t;

typedef enum hny_event_type_t {
    HNY_EVENT_DESTROY,
    HNY_EVENT_CLOSE,
    HNY_EVENT_POSTRESIZE,   // vec2
    HNY_EVENT_FOCUS,
    HNY_EVENT_UNFOCUS,
    HNY_EVENT_KEY,          // key_info
    HNY_EVENT_MOUSE_MOVE,   // vec2
    HNY_EVENT_MOUSE_DELTA,  // vec2
    HNY_EVENT_MOUSE_BUTTON, // button_info
    HNY_EVENT_MOUSE_WHEEL,  // wheel_info
    HNY_EVENT_DPICHANGE,    // dpi_scale
    HNY_EVENT_USERRESIZE    // rect
} hny_event_type_t;

typedef struct hny_event_t {
    hny_event_type_t type;
    union u {
        hny_vec2_t vec2;
        struct {
            hny_key_t key;
            hny_input_action_t action;
        } key_info;
        struct {
            hny_mouse_button_t button;
            hny_input_action_t action;
        } button_info;
        struct {
            float x, y;
        } wheel_info;
        float dpi_scale;
        hny_rect_t rect;
    } u;
} hny_event_t;

typedef struct hny_icon_t hny_icon_t;
typedef struct hny_cursor_t hny_cursor_t;
typedef struct hny_window_t hny_window_t;

typedef void (*hny_window_callback_fn_t)(hny_window_t*, hny_event_t*);

hny_icon_t* hny_icon_create(const char* path, hny_vec2_t size);
hny_cursor_t* hny_cursor_create(const char* path, hny_vec2_t size);
hny_window_t* hny_window_create(hny_vec2_t off, hny_vec2_t size);

void hny_cursor_free(hny_cursor_t* cur);
void hny_icon_free(hny_icon_t* icon);

bool hny_win32_init(void);
hny_window_t* hny_window_create(hny_vec2_t off, hny_vec2_t size);

bool hny_window_destroy(hny_window_t* window);
bool hny_window_free(hny_window_t* window);

void hny_window_set_icon(hny_window_t* window, const hny_icon_t* icon, hny_icon_type_t icont);
void hny_window_set_cursor(hny_window_t* window, hny_cursor_t* cur);
void hny_window_set_callback(hny_window_t* window, hny_window_callback_fn_t callback);
void hny_window_set_title(hny_window_t* window, const char* name);
void hny_window_set_visible(hny_window_t* window, bool vis);
void hny_window_set_state(hny_window_t* window, hny_window_state_t cmd);
void hny_window_set_rect(hny_window_t* window, hny_rect_t rect);

bool hny_window_visible(const hny_window_t* window);
float hny_window_dpi_scale(const hny_window_t* window);
bool hny_window_rect(const hny_window_t* window, hny_rect_t* out);
bool hny_cursor_pos(hny_vec2_t* out);
bool hny_window_valid(const hny_window_t* window);

void hny_dispatch_events(void);

#ifdef __cplusplus
}
#endif

#endif