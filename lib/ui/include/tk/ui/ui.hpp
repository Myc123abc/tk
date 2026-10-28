#pragma once

#include "tk/base.hpp"
#include "tk/variant.hpp"
#include "color.hpp"
#include "transform.hpp"
#include "tween.hpp"
#include "widget.hpp"
#include "tk/string_id.hpp"
#include "tk/rect.hpp"
#include "input.hpp"

#include <string>
#include <optional>
#include <expected>
#include <span>
#include <vector>

namespace tk::ui {

////////////////////////////////////////////////////////////////////////////////
///                              Animation
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

/**
 * get a lerp value in ping pong
 * @param name unique global name
 * @param b drive boolean value
 * @param forward_dur duration (us)
 * @param reverse_dur duration (us)
 * @param ease ease funcation for tween
 * @return lerp value (0.0 ~ 1.0)
 */
auto ping_pong(StringID name, bool b, double forward_dur, double reverse_dur, Tween::Ease ease = Tween::linear) noexcept -> double;
inline auto ping_pong(StringID name, bool b, double duration, Tween::Ease ease = Tween::linear) noexcept { return ping_pong(name, b, duration, duration, ease); }

/**
 * reset tween
 * @param name name of tween
 */
void reset_tween(StringID name) noexcept;

////////////////////////////////////////////////////////////////////////////////
///                              Image
////////////////////////////////////////////////////////////////////////////////

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

enum class CornerFlag
{
  top_left     = 1 << 0,
  top_right    = 1 << 1,
  bottom_left  = 1 << 2,
  bottom_right = 1 << 3,

  none   = 1 << 4,
  top    = top_left    | top_right,
  bottom = bottom_left | bottom_right,
  left   = top_left    | bottom_left,
  right  = top_right   | bottom_right,
  all    = top         | bottom,
};

struct ImageConfig
{
  struct Blur
  {
    float sigma{};
    uint  cnt{};
  };
  struct Rounding
  {
    float            radius{};
    Flag<CornerFlag> flags{};
  };
  Variant<Blur>           blur_cfg;
  std::optional<Rounding> rounding_cfg;

  ImageConfig(Blur blur, float radius = {}, Flag<CornerFlag> flags = {}) noexcept
    : blur_cfg(blur), rounding_cfg({ radius, flags }) {}
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

////////////////////////////////////////////////////////////////////////////////
///                              Text
////////////////////////////////////////////////////////////////////////////////

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

/**
 * load fonts in parallel
 * @param paths font paths
 * @return font info or error for each input path, in input order
 */
auto load_fonts(std::span<std::string_view const> paths) noexcept -> std::vector<std::expected<FontInfo, FontLoadErrorType>>;

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
auto text(StringLiteral text, float2 pos, float size, Color color, TextConfig const& cfg = {}) noexcept -> TextResult;

/**
 * get parse result of text with sepcific config
 * @param text
 * @param size
 * @param cfg text config
 * @return text parse result
 */
auto text(StringLiteral text, float size, TextConfig const& cfg = {}) noexcept -> TextResult;

/**
 * Get bounding rectangle of the text in specific size.
 * @param text
 * @param size
 * @param cfg text config
 * @return bounding rectangle if text generation complete.
 */
auto get_text_bounding_rect(StringLiteral text, float size, TextConfig const& cfg = {}) noexcept -> std::optional<Rect>;

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

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
void begin(StringID name, int x, int y, uint width, uint height, bool* is_closed = {}, WindowConfig const& cfg = {}) noexcept;

// end a window
void end() noexcept;

/**
 * Draw on the current window's title bar during end().
 * The callback origin is the title bar's left-top corner.
 * @param func receives the drawable title bar extent excluding window controls
 */
void draw_on_title_bar(std::move_only_function<void(float2)>&& func) noexcept;

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

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

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

/**
 * discard the pixel of specific shapes
 * @param func function of discard shapes
 */
void discard_beg(std::function<void()> func) noexcept;

/// discard operation over
void discard_end() noexcept;

/**
 * Start a scissor area.
 * @param pos
 * @param extent
 */
void scissor_area_begin(float2 pos, float2 extent) noexcept;

// End a scissor area
void scissor_area_end() noexcept;

/**
 * Start a viewport which can be dragged with the left mouse button and scaled
 * around the cursor with the mouse wheel. Nested viewport bounds are expressed
 * in the parent viewport's content coordinates.
 * @param name
 * @param pos
 * @param extent
 */
void viewport_begin(StringID name, float2 pos, float2 extent) noexcept;

// End the viewport
void viewport_end() noexcept;

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

/**
 * draw a rectangle
 * @param left_top left upper corner
 * @param right_bottom right down corner
 * @param color
 * @param thickness
 * @param rounding corner radius
 * @param flags rounded corner selection
 */
void rectangle(float2 left_top, float2 right_bottom, Color color = {}, float thickness = {}, float rounding = {}, Flag<CornerFlag> flags = {}) noexcept;

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

}
