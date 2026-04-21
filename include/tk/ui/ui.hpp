#pragma once

#include "../util/flag.hpp"
#include "../util/vec.hpp"
#include "tween.hpp"

#include <windows.h>

#include <string_view>
#include <optional>

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                                Misc
////////////////////////////////////////////////////////////////////////////////

struct Color
{
  Color() = default;

  Color(uint32_t color) noexcept
  {
    r = static_cast<float>((color >> 24) & 0xFF) / 255;
    g = static_cast<float>((color >> 16) & 0xFF) / 255;
    b = static_cast<float>((color >> 8 ) & 0xFF) / 255;
    a = static_cast<float>((color      ) & 0xFF) / 255;
  }

  Color(vec4 color) noexcept
    : r(color.x), g(color.y), b(color.z), a(color.w) {}

  Color(float r, float g, float b, float a) noexcept
    : r(r), g(g), b(b), a(a) {}

  operator vec4() noexcept { return { r, g, b, a }; }

  float r{}, g{}, b{}, a{};
};

/**
 * get lerp color
 * @param x begin of color
 * @param y end of color
 * @param v lerp value
 * @return lerp color
 */
auto lerp(Color x, Color y, float v) noexcept -> vec4;

/**
 * get lerp value of two points
 * @param x
 * @param y
 * @param v
 * @return lerp value
 */
auto lerp(vec2 x, vec2 y, float v) noexcept -> vec2;

struct WindowConfig
{
  bool                 display_title_bar{};
  bool                 display_window_shadow{};
  std::optional<Color> wireframe_color{};
  bool                 display_wireframe_only_active{};
  bool                 no_resize{};
  bool                 no_move{};

  struct BlurBackdrop
  {
    enum class Style
    {
      none,
      blur,
      acrylic,
    } style;

    union
    {
      float blur_radius{};
      struct Acrylic
      {
        float opacity{};
        float blur{};
        vec4  tint_color{};
        vec4  luminosity_color{};
      } acrylic;
    };

    void default_blur() noexcept
    {
      style       = Style::blur;
      blur_radius = 4.f;
    }

    void default_acrylic() noexcept
    {
      style                    = Style::acrylic;
      acrylic.opacity          = .02f;
      acrylic.blur             = 30.f;
      acrylic.tint_color       = { .125f, .125f, .125f, .4f };
      acrylic.luminosity_color = { .125f, .125f, .125f, .8f };
    }

    auto operator!=(BlurBackdrop const& b) const noexcept
    {
      if (style != b.style) return true;
      if (style == Style::blur)
        return blur_radius != b.blur_radius;
      else if (style == Style::acrylic)
        return acrylic.opacity          != b.acrylic.opacity    ||
               acrylic.blur             != b.acrylic.blur       ||
               acrylic.tint_color       != b.acrylic.tint_color ||
               acrylic.luminosity_color != b.acrylic.luminosity_color;
      return false;
    }

  } backdrop;
};

using Backdrop      = WindowConfig::BlurBackdrop;
using BackdropStyle = WindowConfig::BlurBackdrop::Style;

/**
 * get a lerp value in ping pong
 * @param name unique global name
 * @param b drive boolean value
 * @param duration duration (us)
 * @param ease ease funcation for tween
 * @return lerp value (0.0 ~ 1.0)
 */
auto ping_pong(std::string_view name, bool b, double duration, Tween::Ease ease = Tween::linear) noexcept -> double;

/**
 * get cursor position
 * @return cursor position
 */
auto get_cursor_pos() noexcept -> vec2;

/**
 * reset tween
 * @param name name of tween
 */
void reset_tween(std::string_view name) noexcept;

/**
 * get extent of image
 * @param path
 * @return width and height of image, if not exist, return (0, 0)
 */
auto image_extent(std::string_view path) noexcept -> vec2;

/**
 * display image
 * @param path
 * @param left_top
 * @param right_bottom
 * @param alpha
 * @return false if image is unexist, or loading, or load failed
 */
auto image(std::string_view path, vec2 left_top, vec2 right_bottom, uint8_t alpha = 0xff) noexcept -> bool;

/**
 * load image
 * @param path
 */
void load_image(std::string_view path) noexcept;

/**
 * load font
 * @param path
 */
void load_font(std::string_view path) noexcept;

enum class FontStyle
{
  regular,
  italic,
  bold,
  italic_bold,
};

/**
 * draw text
 * @param text
 * @param pos left top of text
 * @param size
 * @param color
 * @param style regular(default), italic, bold, italic_bold
 * @return extent of text
 */
auto text(std::string_view text, vec2 pos, float size, Color color, FontStyle style = {}) noexcept -> vec2;

/**
 * draw text
 * @param text
 * @param pos left top of text
 * @param size
 * @param inner_color
 * @param outer_color alpha not 0 then draw outline
 * @return extent of text
 */
auto text(std::string_view text, vec2 pos, float size, Color inner_color, Color outer_color) noexcept -> vec2;

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

/**
 * begin a window
 * @param name window name cannot be duplicated
 * @param x
 * @param y
 * @param width
 * @param height
 * @param is_closed make the window can be closed if is_closed is not nullptr,
 *                  and this is only a flag to indicate whether the window is closed,
 *                  if you want to close the window, stop call the begin and end of this window
 * @param cfg window config
 */
void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed = {}, WindowConfig const& cfg = {}) noexcept;

// end a window
void end() noexcept;

/**
 * set cursor move invalid area
 * every frame rendering will be reset
 * @param x
 * @param y
 * @param width
 * @param height
 */
void add_move_invalid_area(vec2 left_top, vec2 right_bottom) noexcept;

/**
 * get window extent in current update function of window
 * @return extent of window
 */
auto window_extent() noexcept -> vec2;

/**
 * get window extent in current update function of window
 * @return extent of window without titlebar
 */
auto window_drawable_extent() noexcept -> vec2;

/**
 * whether current window is fullscreen
 * @return is window fullscreen
 */
auto is_fullscreen_window() noexcept -> bool;

// fullscreen window
void fullscreen_window() noexcept;

// restore window if fullscreen
void restore_fullscreen_window() noexcept;

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

/**
 * discard the pixel of specific rectangle for last draw shape
 * @param left_top
 * @param right_bottom
 */
void discard_rectangle(vec2 left_top, vec2 right_bottom) noexcept;

/// use path draw between lines and beziers
void begin_path() noexcept;

/**
 * end the path draw
 * @param color
 * @param thickness
 */
void end_path(Color color = {}, float thickness = {}) noexcept;

/// use union operator between shapes
void begin_union() noexcept;

/**
 * end the union operator
 * @param color
 * @param thickness
 */
void end_union(Color color, float thickness = {}) noexcept;

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

/**
 * draw a rectangle
 * @param left_top left upper corner
 * @param right_bottom right down corner
 * @param color
 * @param thickness
 */
void rectangle(vec2 left_top, vec2 right_bottom, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a triangle (clockwise)
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void triangle(vec2 p0, vec2 p1, vec2 p2, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
void circle(vec2 center, float radius, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a line
 * @param p0
 * @param p1
 * @param color
 * @param thickness
 */
void line(vec2 p0, vec2 p1, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw a quadratic bezier
 * @param p0
 * @param p1
 * @param p2
 * @param color
 * @param thickness
 */
void bezier(vec2 p0, vec2 p1, vec2 p2, Color color = {}, float thickness = 1.f) noexcept;

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

struct ButtonState
{
  bool clicked{};
  bool hovered{};
  bool move_out{};

  constexpr operator bool() const noexcept
  {
    return clicked;
  }  
};

/**
 * a button feature can custom shape
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @return button state
 */
auto button(std::string_view name, int x, int y, uint32_t width, uint32_t height) noexcept-> ButtonState;

/**
 * normal button
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param button_color
 * @param button_hover_color
 * @return button state
 */
auto button(
  std::string_view name,
  int              x,
  int              y,
  uint32_t         width,
  uint32_t         height,
  Color            button_color,
  Color            button_hover_color) noexcept-> ButtonState;

/**
 * draw a button, can draw an icon in the center of button
 * default have a color lerp animation when cursor hover on button and leave on it
 * TODO: add bitmap draw replace draw icon by hand
 * @param name name cannot be duplicate in the window
 * @param x
 * @param y
 * @param width
 * @param height
 * @param button_color
 * @param button_hover_color
 * @param mouse_down_color
 * @param icon_update_func the function be called for draw icon by ui draw api
 * @param icon_width
 * @param icon_height
 * @param icon_color
 * @param icon_hover_color
 * @return button state
 */
auto button(
  std::string_view                        name,
  int                                     x,
  int                                     y,
  uint32_t                                width,
  uint32_t                                height,
  Color                                   button_color,
  Color                                   button_hover_color,
  std::optional<Color>                    mouse_down_color,
  std::function<void(uint32_t, uint32_t)> icon_update_func,
  uint32_t                                icon_width,
  uint32_t                                icon_height,
  Color                                   icon_color,
  Color                                   icon_hover_color) noexcept-> ButtonState;

////////////////////////////////////////////////////////////////////////////////
///                               Key
////////////////////////////////////////////////////////////////////////////////

#define KEY_LIST(X)                \
  X(Q,                 'Q')        \
  X(F11,               VK_F11)     \
  X(Shift,             VK_SHIFT)   \
  X(Space,             VK_SPACE)   \
  X(Mouse_Left_Button, VK_LBUTTON)

enum class Key
{
#define X(name, value) name = value,
  KEY_LIST(X)
#undef X
};

Flag(KeyState,
  idle      = 0b0000,
  down      = 0b0001,
  down_idle = 0b0011,
  press     = 0b0101,
  up        = 0b1000,
)

struct GetKeyResult
{
  KeyState state{};

  constexpr operator bool() const noexcept
  {
    return state == KeyState::down ||
           state == KeyState::press;
  }

  auto has_down() const noexcept { return has_flag(state, KeyState::down); }

  auto is_uppercase() const noexcept -> bool;
  auto is_lowercase() const noexcept -> bool { return !is_uppercase(); }
};

/**
 * get key state
 * @param key
 * @return result of key state
 */
auto get_key(Key key) noexcept -> GetKeyResult;

}
