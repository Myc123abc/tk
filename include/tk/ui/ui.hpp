#pragma once

#include "tk/flag.hpp"
#include "tk/base.hpp"
#include "tk/variant.hpp"
#include "color.hpp"
#include "transform.hpp"
#include "tween.hpp"
#include "widget.hpp"

#include <windows.h>

#include <string>
#include <optional>
#include <expected>

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                                Misc
////////////////////////////////////////////////////////////////////////////////

/**
 * get lerp color
 * @param x begin of color
 * @param y end of color
 * @param v lerp value
 * @return lerp color
 */
auto lerp(Color x, Color y, float v) noexcept -> float4;

/**
 * get lerp value of two points
 * @param x
 * @param y
 * @param v
 * @return lerp value
 */
auto lerp(float2 x, float2 y, float v) noexcept -> float2;

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
        float4 tint_color{};
        float4 luminosity_color{};
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
 * @param forward_dur duration (us)
 * @param reverse_dur duration (us)
 * @param ease ease funcation for tween
 * @return lerp value (0.0 ~ 1.0)
 */
auto ping_pong(std::string_view name, bool b, double forward_dur, double reverse_dur, Tween::Ease ease = Tween::linear) noexcept -> double;
inline auto ping_pong(std::string_view name, bool b, double duration, Tween::Ease ease = Tween::linear) noexcept { return ping_pong(name, b, duration, duration, ease); }

/**
 * reset tween
 * @param name name of tween
 */
void reset_tween(std::string_view name) noexcept;

/**
 * get cursor position
 * @return cursor position
 */
auto get_cursor_pos() noexcept -> float2;

/**
 * get cursor position on window
 * @return cursor position
 */
auto get_cursor_pos_on_window() noexcept -> float2;

/**
 * get extent of image
 * @param path
 * @return width and height of image, if not exist, return (0, 0)
 */
auto image_extent(std::string_view path) noexcept -> float2;

namespace ImageLoadError {

struct unexist{};
struct loading{};
struct decode_failed{ std::string_view msg; };

using Type = Variant<
  unexist,
  loading,
  decode_failed
>;

}

using ImageLoadErrorType = ImageLoadError::Type;

struct ImageConfig
{
  struct Blur
  {
    float sigma{};
    uint  cnt{};
  };
  Variant<Blur> cfg;

  static auto blur(float sigma, uint cnt) noexcept { return ImageConfig{ Blur{ sigma, cnt } }; }
};

/**
 * display image
 * @param path
 * @param left_top
 * @param right_bottom
 * @param alpha
 * @param cfg can use for blur image
 * @return false if image is unexist, or loading, or load failed
 */
auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha = 0xff, std::optional<ImageConfig> cfg = {}) noexcept -> std::expected<void, ImageLoadErrorType>;

/**
 * load image
 * @param path
 * @return false if image is unexist, or loading, or load failed
 */
auto load_image(std::string_view path) noexcept -> std::expected<void, ImageLoadErrorType>;

enum class FontStyle
{
  regular,
  italic,
  bold,
  italic_bold,
};

enum class TextDirection
{
  horizontal,
  vertical,
};

struct FontInfo
{
  std::string family;
  FontStyle   style;
};

namespace FontLoadError {

struct unexist {};
struct freetype_err { uint8 code{}; };

using Type = Variant<unexist, freetype_err>;

}

using FontLoadErrorType = FontLoadError::Type;

/**
 * load font
 * @param path
 * @return font info
 */
auto load_font(std::string_view path) noexcept -> std::expected<FontInfo, FontLoadErrorType>;

struct TextConfig
{
  std::string_view family;
  FontStyle        style{};
  TextDirection    direction{};
  Color            outer_color;
  float            outline_width{ 0.15f };
  bool             pos_as_baseline{};
};

struct TextResult
{
  float2 extent;
  float  ascender{};
};

/**
 * draw text
 * @param text
 * @param pos left top of text
 * @param size
 * @param color
 * @param cfg text config
 * @return text parse result
 */
auto text(std::string_view text, float2 pos, float size, Color color, TextConfig cfg = {}) noexcept -> TextResult;

/**
 * get parse result of text with sepcific config
 * @param text
 * @param size
 * @param cfg text config
 * @return text parse result
 */
auto text(std::string_view text, float size, TextConfig cfg = {}) noexcept -> TextResult;

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
void begin(std::string_view name, int x, int y, uint width, uint height, bool* is_closed = {}, WindowConfig const& cfg = {}) noexcept;

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
void add_move_invalid_area(float2 left_top, float2 right_bottom) noexcept;

/**
 * get window extent in current update function of window
 * @return extent of window
 */
auto window_extent() noexcept -> float2;

/**
 * get window extent in current update function of window
 * @return extent of window without titlebar
 */
auto window_drawable_extent() noexcept -> float2;

/**
 * whether current window is fullscreen
 * @return is window fullscreen
 */
auto is_fullscreen_window() noexcept -> bool;

// fullscreen window
void fullscreen_window() noexcept;

// restore window if fullscreen
void restore_fullscreen_window() noexcept;

/**
 * begin a transform scope for following draw commands
 * @param transform affine transform applied in draw order
 */
void transform_beg(Matrix const& transform) noexcept;

/**
 * begin a transform scope for following draw commands
 * @param transform transform builder
 */
void transform_beg(Transform const& transform) noexcept;

/// end current transform scope
void transform_end() noexcept;

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

/**
 * discard the pixel of specific shapes
 * @param func function of discard shapes
 */
void discard_beg(std::function<void()> func) noexcept;

/// discard operation over
void discard_end() noexcept;

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
void rectangle(float2 left_top, float2 right_bottom, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a triangle (clockwise)
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void triangle(float2 p0, float2 p1, float2 p2, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
void circle(float2 center, float radius, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a line
 * @param p0
 * @param p1
 * @param color
 * @param thickness
 */
void line(float2 p0, float2 p1, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw an arc
 * @param center
 * @param p0
 * @param p1
 * @param ccw whether counter clockwise
 * @param color
 * @param thickness
 */
void arc(float2 center, float2 p0, float2 p1, bool ccw, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw a quad bezier
 * @param p0
 * @param p1
 * @param p2
 * @param color
 * @param thickness
 */
void quad_bezier(float2 p0, float2 p1, float2 p2, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw a cubic bezier
 * @param p0
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color = {}, float thickness = 1.f) noexcept;

////////////////////////////////////////////////////////////////////////////////
///                             Path
////////////////////////////////////////////////////////////////////////////////

/**
 * start to draw a path
 * @param p0 start point
 */
void path_begin(float2 p0) noexcept;

/**
 * path line
 * @param p1
 */
void path_line_to(float2 p1) noexcept;

/**
 * path arc
 * @param center
 * @param p1
 * @param ccw whether counter clockwise
 */
void path_arc_to(float2 center, float2 p1, bool ccw) noexcept;

/**
 * path bezier quad draw
 * @param p1
 * @param p2
 */
void path_quad_bezier_to(float2 p1, float2 p2) noexcept;

/**
 * path bezier cubic draw
 * @param p1
 * @param p2
 * @param p3
 */
void path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept;

/**
 * path draw end
 * @param close close with a line
 * @param color
 * @param thickness
 */
void path_end(bool close = true, Color color = {}, float thickness = {}) noexcept;

/// start collecting closed shapes for a union boolean operation
void union_beg() noexcept;

/**
 * draw union result of collected closed shapes
 * @param color
 * @param thickness
 */
void union_end(Color color = {}, float thickness = {}) noexcept;

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
    return state == KeyState::down ||
           state == KeyState::press;
  }

  auto has_down() const noexcept { return state.contains(KeyState::down); }

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
