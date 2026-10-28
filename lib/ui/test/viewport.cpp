#include "tk/tk.hpp"
#include "tk/ui/transform.hpp"
#include "tk/ui/ui.hpp"

#include <algorithm>

using namespace tk;

namespace {

constexpr auto Image_Path = "assets/image/test.png";
constexpr auto Font_Path  = "assets/font/NotoSansJP-Regular.ttf";

constexpr auto Window_Background = 0x0d1117ffu;
constexpr auto Scene_Background  = 0x111824ffu;
constexpr auto Panel_Background  = 0x182230ffu;
constexpr auto Panel_Border      = 0x34465fffu;
constexpr auto Grid_Color        = 0x26364a80u;
constexpr auto Text_Color        = 0xe6edf3ffu;
constexpr auto Muted_Text_Color  = 0x9fb0c3ffu;
constexpr auto Blue              = 0x58a6ffffu;
constexpr auto Green             = 0x3fb950ffu;
constexpr auto Orange            = 0xf0883effu;
constexpr auto Pink              = 0xdb61a2ffu;
constexpr auto Purple            = 0xa371f7ffu;
constexpr auto Yellow            = 0xd29922ffu;

constexpr auto Panel_Extent = float2{ 280, 220 };
constexpr auto Panel_Gap    = 20.f;
constexpr auto Scene_Extent = float2{ 1240, 1040 };

struct ViewportTestState
{
  bool button_enabled{};
  bool nested_button_enabled{};
};

enum class ImageState
{
  loading,
  ready,
  failed,
};

auto image_state() noexcept -> ImageState
{
  static auto error_reported = false;

  auto result = ui::load_image(Image_Path);
  if (result)
    return ImageState::ready;

  auto state = ImageState::loading;
  result.error().visit(
    [](ui::ImageLoadError::loading) {},
    [&](ui::ImageLoadError::unexist)
    {
      state = ImageState::failed;
      if (!error_reported)
        warn("viewport test image does not exist: {}", Image_Path);
    },
    [&](ui::ImageLoadError::decode_failed const& error)
    {
      state = ImageState::failed;
      if (!error_reported)
        warn("failed to decode viewport test image {}: {}", Image_Path, error.msg);
    }
  );
  error_reported |= state == ImageState::failed;
  return state;
}

auto load_test_font() noexcept -> bool
{
  auto result = ui::load_font(Font_Path);
  if (result)
    return true;

  result.error().visit(
    [](ui::FontLoadError::unexist)
    {
      warn("viewport test font does not exist: {}", Font_Path);
    },
    [](ui::FontLoadError::freetype_err const error)
    {
      warn("failed to load viewport test font {}, FreeType error {}", Font_Path, error.code);
    }
  );
  return false;
}

void draw_image(float2 left_top, float2 right_bottom) noexcept
{
  static auto error_reported = false;

  auto result = ui::image(Image_Path, left_top, right_bottom);
  if (result || error_reported)
    return;

  result.error().visit(
    [](ui::ImageLoadError::loading) {},
    [&](ui::ImageLoadError::unexist)
    {
      error_reported = true;
      warn("viewport test image does not exist: {}", Image_Path);
    },
    [&](ui::ImageLoadError::decode_failed const& error)
    {
      error_reported = true;
      warn("failed to decode viewport test image {}: {}", Image_Path, error.msg);
    }
  );
}

void draw_panel(StringLiteral title, float2 pos) noexcept
{
  ui::rectangle(pos, pos + Panel_Extent, Panel_Background);
  ui::rectangle(pos, pos + Panel_Extent, Panel_Border, 1);
  ui::text(title, pos + float2{ 12, 9 }, 16, Text_Color);
  ui::line(pos + float2{ 0, 38 }, pos + float2{ Panel_Extent.x, 38 }, Panel_Border);
}

void draw_rectangles(float2 pos) noexcept
{
  draw_panel("Rectangles", pos);

  ui::rectangle(pos + float2{ 18, 55 }, pos + float2{ 118, 118 }, Blue, 0, 14);
  ui::rectangle(pos + float2{ 145, 55 }, pos + float2{ 260, 118 }, Green, 4, 18);

  auto left_top     = pos + float2{ 72, 146 };
  auto right_bottom = pos + float2{ 208, 193 };
  auto center       = (left_top + right_bottom) / 2;
  ui::transform_beg(ui::Transform{}.rotate(center, -12));
  ui::rectangle(left_top, right_bottom, Orange, 4, 10);
  ui::transform_end();
}

void draw_triangles(float2 pos) noexcept
{
  draw_panel("Triangles", pos);

  ui::triangle(
    pos + float2{ 70, 52 },
    pos + float2{ 125, 145 },
    pos + float2{ 20, 145 },
    Purple
  );
  ui::triangle(
    pos + float2{ 205, 52 },
    pos + float2{ 262, 145 },
    pos + float2{ 150, 145 },
    Pink,
    5
  );

  ui::triangle(
    pos + float2{ 140, 154 },
    pos + float2{ 170, 203 },
    pos + float2{ 110, 203 },
    Yellow,
    2
  );
}

void draw_circles_and_arcs(float2 pos) noexcept
{
  draw_panel("Circles and arcs", pos);

  ui::circle(pos + float2{ 70, 91 }, 38, Green);
  ui::circle(pos + float2{ 200, 91 }, 42, Blue, 5);

  auto center = pos + float2{ 140, 172 };
  ui::arc(center, center + float2{ -62, 0 }, center + float2{ 0, -62 }, false, Orange, 5);
  ui::arc(center, center + float2{ 62, 0 }, center + float2{ 0, 38 }, true, Pink, 3);
}

void draw_lines_and_beziers(float2 pos) noexcept
{
  draw_panel("Lines and Beziers", pos);

  ui::line(pos + float2{ 18, 60 }, pos + float2{ 260, 60 }, Blue);
  ui::line(pos + float2{ 18, 78 }, pos + float2{ 260, 104 }, Green, 5);
  ui::quad_bezier(
    pos + float2{ 18, 150 },
    pos + float2{ 140, 65 },
    pos + float2{ 260, 150 },
    Orange,
    4
  );
  ui::cubic_bezier(
    pos + float2{ 18, 190 },
    pos + float2{ 72, 115 },
    pos + float2{ 205, 230 },
    pos + float2{ 260, 165 },
    Purple,
    4
  );
}

void draw_paths(float2 pos) noexcept
{
  draw_panel("Paths", pos);

  ui::path_begin(pos + float2{ 20, 75 });
  ui::path_line_to(pos + float2{ 105, 52 });
  ui::path_line_to(pos + float2{ 82, 112 });
  ui::path_line_to(pos + float2{ 118, 185 });
  ui::path_line_to(pos + float2{ 55, 152 });
  ui::path_line_to(pos + float2{ 18, 192 });
  ui::path_end(true, Pink);

  ui::path_begin(pos + float2{ 135, 180 });
  ui::path_line_to(pos + float2{ 145, 92 });
  ui::path_quad_bezier_to(pos + float2{ 165, 45 }, pos + float2{ 190, 92 });
  ui::path_arc_to(pos + float2{ 215, 92 }, pos + float2{ 240, 92 }, false);
  ui::path_cubic_bezier_to(
    pos + float2{ 252, 115 },
    pos + float2{ 252, 170 },
    pos + float2{ 262, 185 }
  );
  ui::path_end(false, Blue, 4);
}

void draw_unions(float2 pos) noexcept
{
  draw_panel("Union fill and outline", pos);

  ui::union_beg("viewport-union-fill");
  ui::circle(pos + float2{ 70, 102 }, 43);
  ui::circle(pos + float2{ 116, 102 }, 43);
  ui::rectangle(pos + float2{ 85, 70 }, pos + float2{ 170, 145 });
  ui::triangle(
    pos + float2{ 155, 60 },
    pos + float2{ 220, 145 },
    pos + float2{ 130, 145 }
  );
  ui::union_end(Purple);

  ui::union_beg();
  ui::circle(pos + float2{ 82, 178 }, 25);
  ui::circle(pos + float2{ 116, 178 }, 25);
  ui::path_begin(pos + float2{ 130, 155 });
  ui::path_line_to(pos + float2{ 205, 155 });
  ui::path_line_to(pos + float2{ 245, 200 });
  ui::path_line_to(pos + float2{ 145, 200 });
  ui::path_end();
  ui::union_end(Green, 3);
}

void draw_vector_discard(float2 pos) noexcept
{
  draw_panel("Discard vector shapes", pos);

  auto mask_center = pos + float2{ 140, 132 };
  ui::discard_beg([=]
  {
    ui::circle(mask_center, 62);
    ui::rectangle(pos + float2{ 108, 62 }, pos + float2{ 172, 202 }, {}, 0, 12);
  });
  ui::rectangle(pos + float2{ 20, 55 }, pos + float2{ 260, 205 }, Blue);
  for (auto x = 20.f; x < 270; x += 24)
    ui::line(pos + float2{ x, 55 }, pos + float2{ x - 80, 205 }, Orange, 8);
  ui::discard_end();

  ui::circle(mask_center, 62, Text_Color, 1);
}

void draw_regular_image(float2 pos, ImageState state) noexcept
{
  draw_panel("Image", pos);

  auto left_top     = pos + float2{ 20, 53 };
  auto right_bottom = pos + float2{ 260, 202 };
  if (state == ImageState::ready)
    draw_image(left_top, right_bottom);
  else
  {
    ui::rectangle(left_top, right_bottom, Scene_Background);
    if (state == ImageState::loading)
      ui::text("Loading image...", left_top + float2{ 48, 64 }, 16, Muted_Text_Color);
    else
      ui::text("Image load failed", left_top + float2{ 48, 64 }, 16, Orange);
  }
  ui::rectangle(left_top, right_bottom, Text_Color, 1);
}

void draw_text(float2 pos) noexcept
{
  draw_panel("MSDF text", pos);

  ui::text("Viewport text", pos + float2{ 20, 58 }, 30, Text_Color);

  auto cfg          = ui::TextConfig{};
  cfg.outer_color   = Purple;
  cfg.outline_width = .22f;
  ui::text("Outlined MSDF", pos + float2{ 20, 104 }, 26, Text_Color, cfg);

  auto text_pos = pos + float2{ 25, 180 };
  ui::transform_beg(ui::Transform{}.rotate(text_pos, -8));
  ui::text("Rotated text at any zoom", text_pos, 18, Green);
  ui::transform_end();
}

void draw_image_discard(float2 pos, ImageState state) noexcept
{
  draw_panel("Discard image", pos);

  auto left_top     = pos + float2{ 20, 53 };
  auto right_bottom = pos + float2{ 260, 202 };
  if (state == ImageState::ready)
  {
    ui::discard_beg([=]
    {
      ui::circle(pos + float2{ 105, 128 }, 64);
      ui::rectangle(pos + float2{ 145, 76 }, pos + float2{ 245, 180 }, {}, 0, 18);
    });
    draw_image(left_top, right_bottom);
    ui::discard_end();
  }
  else
  {
    ui::rectangle(left_top, right_bottom, Scene_Background);
    ui::text("Waiting for image", left_top + float2{ 48, 64 }, 16, Muted_Text_Color);
  }
  ui::rectangle(left_top, right_bottom, Text_Color, 1);
}

void draw_text_discard(float2 pos) noexcept
{
  draw_panel("Discard text", pos);

  ui::discard_beg([=]
  {
    ui::circle(pos + float2{ 140, 132 }, 72);
    ui::rectangle(pos + float2{ 35, 105 }, pos + float2{ 245, 150 });
  });
  ui::text("CLIPPED", pos + float2{ 30, 70 }, 48, Blue);
  ui::text("MSDF TEXT", pos + float2{ 23, 132 }, 36, Pink);
  ui::discard_end();

  ui::circle(pos + float2{ 140, 132 }, 72, Panel_Border, 1);
}

void draw_aa_references(float2 pos) noexcept
{
  draw_panel("AA and intersections", pos);

  for (auto i = 0; i < 3; ++i)
  {
    auto y = 60.f + i * 24.f;
    ui::line(pos + float2{ 20, y }, pos + float2{ 260, y }, Text_Color, static_cast<float>(i + 1));
  }
  ui::line(pos + float2{ 20, 132 }, pos + float2{ 260, 132 }, Blue, .5f);

  ui::path_begin(pos + float2{ 20, 166 });
  ui::path_line_to(pos + float2{ 72, 142 });
  ui::path_line_to(pos + float2{ 78, 166 });
  ui::path_line_to(pos + float2{ 134, 142 });
  ui::path_line_to(pos + float2{ 140, 166 });
  ui::path_line_to(pos + float2{ 195, 142 });
  ui::path_line_to(pos + float2{ 260, 166 });
  ui::path_end(false, Yellow, 1);

  ui::path_begin(pos + float2{ 20, 202 });
  ui::path_line_to(pos + float2{ 75, 176 });
  ui::path_line_to(pos + float2{ 82, 202 });
  ui::path_line_to(pos + float2{ 140, 176 });
  ui::path_line_to(pos + float2{ 147, 202 });
  ui::path_line_to(pos + float2{ 205, 176 });
  ui::path_line_to(pos + float2{ 260, 202 });
  ui::path_end(false, Pink, 6);
}

void draw_nested_viewport(StringID viewport_name, StringID button_name, bool& button_enabled, float2 pos) noexcept
{
  constexpr auto viewport_extent = float2{ 360, 150 };
  constexpr auto content_extent = float2{ 680, 320 };

  ui::viewport_begin(viewport_name, pos, viewport_extent);
  ui::scissor_area_begin(pos, viewport_extent);

  ui::rectangle(pos, pos + content_extent, Panel_Background);
  for (auto x = 0.f; x <= content_extent.x; x += 32)
    ui::line(pos + float2{ x, 0 }, pos + float2{ x, content_extent.y }, Grid_Color);
  for (auto y = 0.f; y <= content_extent.y; y += 32)
    ui::line(pos + float2{ 0, y }, pos + float2{ content_extent.x, y }, Grid_Color);

  ui::text("Nested viewport", pos + float2{ 20, 14 }, 20, Text_Color);
  ui::text("Drag and zoom independently", pos + float2{ 20, 40 }, 13, Muted_Text_Color);

  auto const button_pos = pos + float2{ 20, 72 };
  auto const button_color = button_enabled ? Green : Blue;
  if (ui::button(button_name, button_pos.x, button_pos.y, 150, 36, button_color, Pink, Orange))
    button_enabled = !button_enabled;
  if (button_enabled)
    ui::text("Nested button on", button_pos + float2{ 16, 8 }, 13, Text_Color);
  else
    ui::text("Nested button off", button_pos + float2{ 16, 8 }, 13, Text_Color);

  ui::circle(pos + float2{ 260, 90 }, 42, Yellow, 4);

  ui::scissor_area_end();
  ui::viewport_end();
  ui::rectangle(pos, pos + viewport_extent, Yellow, 2);
}

void draw_scene(
  StringLiteral      title,
  StringID           button_name,
  StringID           nested_viewport_name,
  StringID           nested_button_name,
  ViewportTestState& viewport_state,
  float2             origin,
  ImageState         state) noexcept
{
  ui::rectangle(origin, origin + Scene_Extent, Scene_Background);

  for (auto x = 0.f; x <= Scene_Extent.x; x += 50)
    ui::line(origin + float2{ x, 0 }, origin + float2{ x, Scene_Extent.y }, Grid_Color);
  for (auto y = 0.f; y <= Scene_Extent.y; y += 50)
    ui::line(origin + float2{ 0, y }, origin + float2{ Scene_Extent.x, y }, Grid_Color);

  ui::text(title, origin + float2{ 20, 14 }, 24, Text_Color);
  ui::text("Zoom deeply and inspect the physical AA width of each item.", origin + float2{ 20, 43 }, 14, Muted_Text_Color);

  auto const button_pos = origin + float2{ 430, 14 };
  auto const button_color = viewport_state.button_enabled ? Green : Blue;
  if (ui::button(button_name, button_pos.x, button_pos.y, 130, 32, button_color, Pink, Orange))
    viewport_state.button_enabled = !viewport_state.button_enabled;
  if (viewport_state.button_enabled)
    ui::text("Button on", button_pos + float2{ 24, 6 }, 14, Text_Color);
  else
    ui::text("Button off", button_pos + float2{ 24, 6 }, 14, Text_Color);

  draw_nested_viewport(
    nested_viewport_name,
    nested_button_name,
    viewport_state.nested_button_enabled,
    origin + float2{ 20, 76 }
  );

  auto panel_pos = [&](uint column, uint row)
  {
    return origin + float2{
      20 + column * (Panel_Extent.x + Panel_Gap),
      250 + row * (Panel_Extent.y + Panel_Gap),
    };
  };

  draw_rectangles(panel_pos(0, 0));
  draw_triangles(panel_pos(1, 0));
  draw_circles_and_arcs(panel_pos(2, 0));
  draw_lines_and_beziers(panel_pos(3, 0));

  draw_paths(panel_pos(0, 1));
  draw_unions(panel_pos(1, 1));
  draw_vector_discard(panel_pos(2, 1));
  draw_regular_image(panel_pos(3, 1), state);

  draw_text(panel_pos(0, 2));
  draw_image_discard(panel_pos(1, 2), state);
  draw_text_discard(panel_pos(2, 2));
  draw_aa_references(panel_pos(3, 2));
}

void draw_viewport(
  StringID           name,
  StringLiteral      title,
  StringID           button_name,
  StringID           nested_viewport_name,
  StringID           nested_button_name,
  ViewportTestState& viewport_state,
  float2             pos,
  float2             extent,
  ImageState         state) noexcept
{
  ui::viewport_begin(name, pos, extent);
  ui::scissor_area_begin(pos, extent);
  draw_scene(title, button_name, nested_viewport_name, nested_button_name, viewport_state, pos + float2{ 20, 20 }, state);
  ui::scissor_area_end();
  ui::viewport_end();
  ui::rectangle(pos, pos + extent, Panel_Border, 1);
}

}

int main()
{
  tk::init();
  if (!load_test_font())
  {
    tk::destroy();
    return 1;
  }

  auto is_closed = false;
  auto left_viewport_state = ViewportTestState{};
  auto right_viewport_state = ViewportTestState{};
  auto config = ui::WindowConfig{
    .display_title_bar = true,
    .display_window_shadow = true,
  };

  while (!is_closed)
  {
    ui::begin("Viewport shape test", 100, 80, 1280, 860, &is_closed, config);

    auto window_extent = ui::window_drawable_extent();
    ui::rectangle({}, window_extent, Window_Background);
    ui::text("Viewport rendering test", { 20, 12 }, 24, Text_Color);
    ui::text("Left drag to pan. Use the mouse wheel to zoom around the cursor.", { 20, 42 }, 14, Muted_Text_Color);

    constexpr auto viewport_gap = 12.f;
    auto viewport_pos = float2{ 20, 70 };
    auto viewport_extent = float2{
      std::max(1.f, (window_extent.x - 40 - viewport_gap) / 2),
      std::max(1.f, window_extent.y - 90),
    };
    auto right_viewport_pos = viewport_pos + float2{ viewport_extent.x + viewport_gap, 0 };
    auto state = image_state();

    draw_viewport(
      "left shapes viewport",
      "Left viewport",
      "left viewport button",
      "left nested viewport",
      "left nested viewport button",
      left_viewport_state,
      viewport_pos,
      viewport_extent,
      state
    );
    draw_viewport(
      "right shapes viewport",
      "Right viewport",
      "right viewport button",
      "right nested viewport",
      "right nested viewport button",
      right_viewport_state,
      right_viewport_pos,
      viewport_extent,
      state
    );

    ui::end();
    tk::update();
  }

  tk::destroy();
}
