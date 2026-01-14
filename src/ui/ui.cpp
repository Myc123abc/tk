#include "ui/ui.hpp"
#include "ui_context.hpp"

using namespace tk::renderer;

namespace {

auto get_bounding_rectangle(std::vector<glm::vec2> const& data) noexcept -> std::pair<glm::vec2, glm::vec2>
{
  assert(data.size() > 1);

  auto min = data[0];
  auto max = data[0];

  for (auto i = 1; i < data.size(); ++i)
  {
    auto& p = data[i];
    if (p.x < min.x) min.x = p.x;
    if (p.y < min.y) min.y = p.y;
    if (p.x > max.x) max.x = p.x;
    if (p.y > max.y) max.y = p.y;
  }

  if (max.x == min.x)
  {
    if (min.x > 1.f)
      --min.x;
    else
      ++max.x;
  }

  if (max.y == min.y)
  {
    if (min.y > 1.f)
      --min.y;
    else
      ++max.y;
  }

  return { min, max };
}

}

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
///                              Misc
////////////////////////////////////////////////////////////////////////////////

void render() noexcept
{
	g_ui_ctx.render();
}

auto color_lerp(Color x, Color y, float v) noexcept -> glm::vec4
{
  return
  {
    std::lerp(x.r, y.r, v),
    std::lerp(x.g, y.g, v),
    std::lerp(x.b, y.b, v),
    std::lerp(x.a, y.a, v)
  };
}

auto delta_time() noexcept -> double
{
  return g_ui_ctx.delta_time();
}

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg) noexcept
{
	g_ui_ctx.begin(name, x, y, width, height, is_closed, cfg);
}

void end() noexcept
{
	g_ui_ctx.end();
}

auto get_mouse_state() -> window::MouseState
{
  return g_ui_ctx.get_mouse_state();
}

void add_move_invalid_area(glm::vec2 left_top, glm::vec2 right_bottom) noexcept
{
  g_ui_ctx.check_draw();
  if (!g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }
  g_ui_ctx._window->add_move_invald_areas(
  {
    static_cast<LONG>(left_top.x),
    static_cast<LONG>(left_top.y),
    static_cast<LONG>(right_bottom.x),
    static_cast<LONG>(right_bottom.y)
  });
}

auto window_extent() noexcept -> glm::vec2
{
  g_ui_ctx.check_draw();
  return { g_ui_ctx._window->snap.width, g_ui_ctx._window->snap.height };
}

auto window_drawable_extent() noexcept -> glm::vec2
{
  auto extent = window_extent();
  if (g_ui_ctx._window->cfg.display_title_bar)
    extent.y -= Title_Bar_Height;
  return extent;
}

////////////////////////////////////////////////////////////////////////////////
///                            Shape Operator
////////////////////////////////////////////////////////////////////////////////

void discard_rectangle(glm::vec2 left_top, glm::vec2 right_bottom) noexcept
{
  g_ui_ctx.check_draw();
  
  auto render_data = g_ui_ctx.get_render_data();
  err_if(render_data->shape_properties.empty(), "failed must draw a shape then use discard rectangle");
  err_if(g_ui_ctx.is_union_draw(), "don't use discard rectangle in union operator, I'm not test for this");
  err_if(g_ui_ctx.is_path_draw(), "don't use discard rectangle in part draw, I'm not test for this");

  auto& shape_property = render_data->shape_properties.back();
  shape_property.set_operator(ShapeProperty::Operator::discard);

  auto offset = g_ui_ctx.get_render_pos();
  left_top     += offset;
  right_bottom += offset;

  g_ui_ctx.add_shape_property(ShapeProperty::Type::rectangle, {}, {}, { left_top.x, left_top.y, right_bottom.x, right_bottom.y });
}

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

void rectangle(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
	left_top     += offset;
	right_bottom += offset;

	g_ui_ctx.add_shape(ShapeProperty::Type::rectangle, color, thickness, { left_top.x, left_top.y, right_bottom.x, right_bottom.y }, { left_top, right_bottom });
}

void triangle(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
	p0 += offset;
	p1 += offset;
	p2 += offset;

	g_ui_ctx.add_shape(ShapeProperty::Type::triangle, color, thickness, { p0.x, p0.y, p1.x, p1.y, p2.x, p2.y }, get_bounding_rectangle({ p0, p1, p2 }));
}

void circle(glm::vec2 center, float radius, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	g_ui_ctx.check_path_not_draw();

	auto offset = g_ui_ctx.get_render_pos();
  center += offset;

  auto r = radius - 1;
  if (r < 0) r = 1;
  g_ui_ctx.add_shape(ShapeProperty::Type::circle, color, thickness, { center.x, center.y, r }, { center - radius, center + radius });
}

void line(glm::vec2 p0, glm::vec2 p1, Color color) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;

  if (g_ui_ctx.is_path_draw())
  {
    g_ui_ctx.path_data[0] = std::bit_cast<float>(std::bit_cast<uint32_t>(g_ui_ctx.path_data[0]) + 1);
    auto points = { p0, p1 };
    g_ui_ctx.path_points.append_range(points);
    g_ui_ctx.path_data.emplace_back(std::bit_cast<float>(ShapeProperty::Type::path_line));
    g_ui_ctx.path_data.append_range(std::ranges::to<std::vector<float>>(points
      | std::views::transform([](auto const& p) { return std::array<float, 2>{ p.x, p.y }; })
      | std::views::join));
  }
  else
    g_ui_ctx.add_shape(ShapeProperty::Type::line, color, {}, { p0.x, p0.y, p1.x, p1.y }, get_bounding_rectangle({ p0, p1 }));
}

void bezier(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color) noexcept
{
	g_ui_ctx.check_draw();

	auto offset = g_ui_ctx.get_render_pos();
  p0 += offset;
  p1 += offset;
  p2 += offset;

  if (g_ui_ctx.is_path_draw())
  {
    g_ui_ctx.path_data[0] = std::bit_cast<float>(std::bit_cast<uint32_t>(g_ui_ctx.path_data[0]) + 1);
    auto points = { p0, p1, p2 };
    g_ui_ctx.path_points.append_range(points);
    g_ui_ctx.path_data.emplace_back(std::bit_cast<float>(ShapeProperty::Type::path_bezier));
    g_ui_ctx.path_data.append_range(std::ranges::to<std::vector<float>>(points
      | std::views::transform([](auto const& p) { return std::array<float, 2>{ p.x, p.y }; })
      | std::views::join));
  }
  else
    g_ui_ctx.add_shape(ShapeProperty::Type::bezier, color, {}, { p0.x, p0.y, p1.x, p1.y, p2.x, p2.y }, get_bounding_rectangle({ p0, p1, p2 }));
}

////////////////////////////////////////////////////////////////////////////////
///                               Widget
////////////////////////////////////////////////////////////////////////////////

auto is_hover_on(std::string_view name, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  return g_ui_ctx.is_hover_on(g_ui_ctx.generic_id(name), left_top, right_bottom);
}

auto is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  return g_ui_ctx.is_hover_on(id, left_top, right_bottom);
}

auto is_hover_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  if (!g_ui_ctx._window->is_cursor_valid_area()) return false;
  auto p = g_ui_ctx._window->cursor_pos();
  return g_ui_ctx.cursor_on_window == g_ui_ctx._window->snap.handle &&
         !g_ui_ctx._window->snap.moving_from_maximize               &&
         point_on(p, left_top, right_bottom);
}

auto is_click_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  g_ui_ctx.check_draw();
  auto window = g_ui_ctx._window;
  if (!window->is_active()             ||
      !window->is_cursor_valid_area()  ||
		   window->is_moving_or_resizing() ||
      !g_ui_ctx.mouse_down_pos         ||
      !g_ui_ctx.mouse_up_pos           ||
       g_ui_ctx.mouse_down_window != g_ui_ctx.mouse_up_window) return false;
  return !g_ui_ctx.is_moving_from_maximize                                 &&
         point_on(g_ui_ctx.mouse_down_pos.value(), left_top, right_bottom) &&
         point_on(g_ui_ctx.mouse_up_pos.value(),   left_top, right_bottom);
}

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
  Color                                   icon_hover_color) noexcept-> bool
{
  auto id = g_ui_ctx.generic_id(name);

  // what is a button
  // button is a rectangle with width and height in specific position
  auto left_top     = glm::vec<2, int>{ x, y };
  auto right_bottom = glm::vec<2, int>{ x + width, y + height };
  if (!g_ui_ctx.draw_title_bar)
  {
    left_top.y     += Title_Bar_Height;
    right_bottom.y += Title_Bar_Height;
  }

  // when cursor hover on it, it will change color to hovered color
  auto is_hovered = ui::is_hover_on(name, left_top, right_bottom);
  if (is_hovered && g_ui_ctx.mouse_down_pos)
    is_hovered = point_on(g_ui_ctx.mouse_down_pos.value(), left_top, right_bottom) &&
                 g_ui_ctx.mouse_down_window == g_ui_ctx._window->snap.handle;

  auto value = g_ui_ctx.ping_pong_lerp(is_hovered, id, 200'000);
  button_color = color_lerp(button_color, button_hover_color, value);

  // when mouse down, color also change
  if (mouse_down_color &&
      is_hovered       &&
      (ui::get_mouse_state() &  window::MouseState::left_button_down ||
       ui::get_mouse_state() == window::MouseState::left_button_press))
    button_color = mouse_down_color.value();

  // draw button
  ui::rectangle({ x, y }, { x + width, y + height }, button_color);

  // draw icon
  if (icon_update_func)
  {
    icon_color = color_lerp(icon_color, icon_hover_color, value);
    auto x_offset = (width  - icon_width)  / 2;
    auto y_offset = (height - icon_height) / 2;
    
    g_ui_ctx.render_on(x + x_offset, y + y_offset, [&]
    {
      g_ui_ctx.enable_tmp_color(icon_color);
      icon_update_func(icon_width, icon_height);
      g_ui_ctx.disable_tmp_color();
    });
  }

  return is_hovered && ui::is_click_on(left_top, right_bottom);
}

}}
