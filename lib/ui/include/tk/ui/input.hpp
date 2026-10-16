#pragma once

#include "tk/flag.hpp"

#include <windows.h>

namespace tk::ui {

#define KEY_LIST(Entry)                              \
  Entry(Mouse_Left_Button,   VK_LBUTTON)             \
  Entry(Mouse_Right_Button,  VK_RBUTTON)             \
  Entry(Mouse_Cancel,        VK_CANCEL)              \
  Entry(Mouse_Middle_Button, VK_MBUTTON)             \
  Entry(Mouse_X1_Button,     VK_XBUTTON1)            \
  Entry(Mouse_X2_Button,     VK_XBUTTON2)            \
  Entry(Backspace,           VK_BACK)                \
  Entry(Tab,                 VK_TAB)                 \
  Entry(Clear,               VK_CLEAR)               \
  Entry(Return,              VK_RETURN)              \
  Entry(Shift,               VK_SHIFT)               \
  Entry(Control,             VK_CONTROL)             \
  Entry(Alt,                 VK_MENU)                \
  Entry(Pause,               VK_PAUSE)               \
  Entry(Caps_Lock,           VK_CAPITAL)             \
  Entry(Kana,                VK_KANA)                \
  Entry(Ime_On,              VK_IME_ON)              \
  Entry(Junja,               VK_JUNJA)               \
  Entry(Final,               VK_FINAL)               \
  Entry(Hanja,               VK_HANJA)               \
  Entry(Ime_Off,             VK_IME_OFF)             \
  Entry(Escape,              VK_ESCAPE)              \
  Entry(Convert,             VK_CONVERT)             \
  Entry(Nonconvert,          VK_NONCONVERT)          \
  Entry(Accept,              VK_ACCEPT)              \
  Entry(Modechange,          VK_MODECHANGE)          \
  Entry(Space,               VK_SPACE)               \
  Entry(Page_Up,             VK_PRIOR)               \
  Entry(Page_Down,           VK_NEXT)                \
  Entry(End,                 VK_END)                 \
  Entry(Home,                VK_HOME)                \
  Entry(Left,                VK_LEFT)                \
  Entry(Up,                  VK_UP)                  \
  Entry(Right,               VK_RIGHT)               \
  Entry(Down,                VK_DOWN)                \
  Entry(Select,              VK_SELECT)              \
  Entry(Print,               VK_PRINT)               \
  Entry(Execute,             VK_EXECUTE)             \
  Entry(Print_Screen,        VK_SNAPSHOT)            \
  Entry(Insert,              VK_INSERT)              \
  Entry(Delete,              VK_DELETE)              \
  Entry(Help,                VK_HELP)                \
  Entry(D0,                  '0')                    \
  Entry(D1,                  '1')                    \
  Entry(D2,                  '2')                    \
  Entry(D3,                  '3')                    \
  Entry(D4,                  '4')                    \
  Entry(D5,                  '5')                    \
  Entry(D6,                  '6')                    \
  Entry(D7,                  '7')                    \
  Entry(D8,                  '8')                    \
  Entry(D9,                  '9')                    \
  Entry(A,                   'A')                    \
  Entry(B,                   'B')                    \
  Entry(C,                   'C')                    \
  Entry(D,                   'D')                    \
  Entry(E,                   'E')                    \
  Entry(F,                   'F')                    \
  Entry(G,                   'G')                    \
  Entry(H,                   'H')                    \
  Entry(I,                   'I')                    \
  Entry(J,                   'J')                    \
  Entry(K,                   'K')                    \
  Entry(L,                   'L')                    \
  Entry(M,                   'M')                    \
  Entry(N,                   'N')                    \
  Entry(O,                   'O')                    \
  Entry(P,                   'P')                    \
  Entry(Q,                   'Q')                    \
  Entry(R,                   'R')                    \
  Entry(S,                   'S')                    \
  Entry(T,                   'T')                    \
  Entry(U,                   'U')                    \
  Entry(V,                   'V')                    \
  Entry(W,                   'W')                    \
  Entry(X,                   'X')                    \
  Entry(Y,                   'Y')                    \
  Entry(Z,                   'Z')                    \
  Entry(Left_Windows,        VK_LWIN)                \
  Entry(Right_Windows,       VK_RWIN)                \
  Entry(Apps,                VK_APPS)                \
  Entry(Sleep,               VK_SLEEP)               \
  Entry(Numpad_0,            VK_NUMPAD0)             \
  Entry(Numpad_1,            VK_NUMPAD1)             \
  Entry(Numpad_2,            VK_NUMPAD2)             \
  Entry(Numpad_3,            VK_NUMPAD3)             \
  Entry(Numpad_4,            VK_NUMPAD4)             \
  Entry(Numpad_5,            VK_NUMPAD5)             \
  Entry(Numpad_6,            VK_NUMPAD6)             \
  Entry(Numpad_7,            VK_NUMPAD7)             \
  Entry(Numpad_8,            VK_NUMPAD8)             \
  Entry(Numpad_9,            VK_NUMPAD9)             \
  Entry(Numpad_Multiply,     VK_MULTIPLY)            \
  Entry(Numpad_Add,          VK_ADD)                 \
  Entry(Numpad_Separator,    VK_SEPARATOR)           \
  Entry(Numpad_Subtract,     VK_SUBTRACT)            \
  Entry(Numpad_Decimal,      VK_DECIMAL)             \
  Entry(Numpad_Divide,       VK_DIVIDE)              \
  Entry(F1,                  VK_F1)                  \
  Entry(F2,                  VK_F2)                  \
  Entry(F3,                  VK_F3)                  \
  Entry(F4,                  VK_F4)                  \
  Entry(F5,                  VK_F5)                  \
  Entry(F6,                  VK_F6)                  \
  Entry(F7,                  VK_F7)                  \
  Entry(F8,                  VK_F8)                  \
  Entry(F9,                  VK_F9)                  \
  Entry(F10,                 VK_F10)                 \
  Entry(F11,                 VK_F11)                 \
  Entry(F12,                 VK_F12)                 \
  Entry(F13,                 VK_F13)                 \
  Entry(F14,                 VK_F14)                 \
  Entry(F15,                 VK_F15)                 \
  Entry(F16,                 VK_F16)                 \
  Entry(F17,                 VK_F17)                 \
  Entry(F18,                 VK_F18)                 \
  Entry(F19,                 VK_F19)                 \
  Entry(F20,                 VK_F20)                 \
  Entry(F21,                 VK_F21)                 \
  Entry(F22,                 VK_F22)                 \
  Entry(F23,                 VK_F23)                 \
  Entry(F24,                 VK_F24)                 \
  Entry(Num_Lock,            VK_NUMLOCK)             \
  Entry(Scroll_Lock,         VK_SCROLL)              \
  Entry(Left_Shift,          VK_LSHIFT)              \
  Entry(Right_Shift,         VK_RSHIFT)              \
  Entry(Left_Control,        VK_LCONTROL)            \
  Entry(Right_Control,       VK_RCONTROL)            \
  Entry(Left_Alt,            VK_LMENU)               \
  Entry(Right_Alt,           VK_RMENU)               \
  Entry(Browser_Back,        VK_BROWSER_BACK)        \
  Entry(Browser_Forward,     VK_BROWSER_FORWARD)     \
  Entry(Browser_Refresh,     VK_BROWSER_REFRESH)     \
  Entry(Browser_Stop,        VK_BROWSER_STOP)        \
  Entry(Browser_Search,      VK_BROWSER_SEARCH)      \
  Entry(Browser_Favorites,   VK_BROWSER_FAVORITES)   \
  Entry(Browser_Home,        VK_BROWSER_HOME)        \
  Entry(Volume_Mute,         VK_VOLUME_MUTE)         \
  Entry(Volume_Down,         VK_VOLUME_DOWN)         \
  Entry(Volume_Up,           VK_VOLUME_UP)           \
  Entry(Media_Next,          VK_MEDIA_NEXT_TRACK)    \
  Entry(Media_Previous,      VK_MEDIA_PREV_TRACK)    \
  Entry(Media_Stop,          VK_MEDIA_STOP)          \
  Entry(Media_Play_Pause,    VK_MEDIA_PLAY_PAUSE)    \
  Entry(Launch_Mail,         VK_LAUNCH_MAIL)         \
  Entry(Launch_Media,        VK_LAUNCH_MEDIA_SELECT) \
  Entry(Launch_App1,         VK_LAUNCH_APP1)         \
  Entry(Launch_App2,         VK_LAUNCH_APP2)         \
  Entry(Oem_1,               VK_OEM_1)               \
  Entry(Oem_Plus,            VK_OEM_PLUS)            \
  Entry(Oem_Comma,           VK_OEM_COMMA)           \
  Entry(Oem_Minus,           VK_OEM_MINUS)           \
  Entry(Oem_Period,          VK_OEM_PERIOD)          \
  Entry(Oem_2,               VK_OEM_2)               \
  Entry(Oem_3,               VK_OEM_3)               \
  Entry(Oem_4,               VK_OEM_4)               \
  Entry(Oem_5,               VK_OEM_5)               \
  Entry(Oem_6,               VK_OEM_6)               \
  Entry(Oem_7,               VK_OEM_7)               \
  Entry(Oem_8,               VK_OEM_8)               \
  Entry(Oem_102,             VK_OEM_102)             \
  Entry(Process_Key,         VK_PROCESSKEY)          \
  Entry(Packet,              VK_PACKET)              \
  Entry(Attn,                VK_ATTN)                \
  Entry(Crsel,               VK_CRSEL)               \
  Entry(Exsel,               VK_EXSEL)               \
  Entry(Erase_Eof,           VK_EREOF)               \
  Entry(Play,                VK_PLAY)                \
  Entry(Zoom,                VK_ZOOM)                \
  Entry(Pa1,                 VK_PA1)                 \
  Entry(Oem_Clear,           VK_OEM_CLEAR)

enum class Key
{
#define X(name, value) name = value,
  KEY_LIST(X)
#undef X
};

enum class KeyState
{
  idle      = 0b0000,
  down      = 0b0001,
  down_idle = 0b0011,
  press     = 0b0101,
  up        = 0b1000,
};

struct GetKeyResult
{
  Flag<KeyState> state{};

  constexpr operator bool() const noexcept
  {
    return state == KeyState::down || state == KeyState::press;
  }

  auto has_down() const noexcept { return state.contains(KeyState::down); }
  auto is_down()  const noexcept { return state == KeyState::down;        }

  auto is_uppercase() const noexcept -> bool;
  auto is_lowercase() const noexcept -> bool { return !is_uppercase(); }
};

/**
 * Get key state
 * @param key
 * @return result of key state
 */
auto get_key(Key key) noexcept -> GetKeyResult;

/**
 * Get the vertical mouse-wheel delta accumulated for this frame.
 * One wheel detent is 1.0; positive values scroll up and negative values scroll down.
 */
auto get_mouse_wheel_delta() noexcept -> float;

}
