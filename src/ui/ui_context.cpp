#include "ui_context.hpp"
#include "../util/error_handling.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/renderer.hpp"
#include "../util/hash.hpp"

using namespace tk::renderer;

namespace tk { namespace ui {

void UIContext::begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg) noexcept
{
  err_if(_call_begin, "begin is called but end not be called");
  _call_begin = true;

  // create window if not have
  if (!_windows.contains(name.data()))
  {
    auto handle = g_wnd_mgr.create_window(x, y, width, height);
    _window = &_windows[name.data()];
    _window->handle        = handle;
    _window->can_be_closed = is_closed;
  }

  _window = &_windows[name.data()];
  _window->is_called = true;
  _window->cfg       = cfg;
  if (is_closed)
    *is_closed = _window->is_closed;

  _shape_properties_offset = 0;
  _idx_beg                 = 0;

  // TODO: use g_wnd_mgr message send to ui context for thread safe
  window = g_wnd_mgr.window(_window->handle);

  if (_window->cfg.display_title_bar)
    set_render_pos(0, Title_Bar_Height);
}

void UIContext::end() noexcept
{
  err_if(!_call_begin, "begin is not called but end is called");
  _call_begin = false;
}

void UIContext::check_draw() const noexcept
{
  err_if(!_call_begin, "not called begin to draw something");
}

void UIContext::check_path_draw() const noexcept
{
  err_if(!_path_begin, "not called path begin to draw path");
}

void UIContext::check_path_not_draw() const noexcept
{
  err_if(_path_begin, "calling path begin to draw path, cannot be used in non-path draw");
}

void UIContext::render() noexcept
{
  // add render data and destroy closed window
  for (auto it = _windows.begin(); it != _windows.end();)
  {
    auto& window = it->second;
    if (window.is_called)
    {
      // TODO: move add title bar to ui::end
      //       and some clear and init to ui::begin
      // if (window.cfg.display_title_bar)
      //   add_title_bar();
      window.is_called = false;
      ++it;
    }
    else
    {
      // destroy
      g_wnd_mgr.close_window(window.handle);
      it = _windows.erase(it);
    }
  }

  // acquire frame, for synchronous with present vsync
  g_renderer.acquire_frame();

  // wakeup if renderer is sleeping, which is no render data has
  auto need_wakeup = g_renderer.is_sleeping();
  auto has_render  = false;

  // process window render datas
  for (auto it = _windows.begin(); it != _windows.end(); ++it)
  {
    auto& window = it->second;
    if (g_renderer.render(window.handle, window.data()))
    {
      window.next_frame();
      has_render = true;
    }
  }

  // wakeup renderer if prev frame no render data
  if (need_wakeup && has_render)
    g_renderer.wakeup();

  // process message queue
  _msg_queue.process(MessageHandler{ g_ui_ctx });

  // update state
  update();
}

void UIContext::update() noexcept
{
  // clear state
  _hovered_widget_ids.clear();
  _ids.clear();
  cursor_on_window = {};
  if (mouse_up_window)
  {
    mouse_down_window = {};
    mouse_up_window   = {};
    mouse_down_pos    = {};
    mouse_up_pos      = {};
  }

  // update which window on top of the cursor
  auto z_orders   = g_wnd_mgr.get_window_z_orders();
  auto cursor_pos = get_cursor_pos();
  if (auto it = std::ranges::find_if(z_orders, [=](auto handle) { return g_wnd_mgr.window(handle)->contains_point(cursor_pos); });
      it != z_orders.end())
    cursor_on_window = *it;

  // update mouse state
  for (auto const& [_, wnd] : _windows)
  {
    auto window = g_wnd_mgr.window(wnd.handle);
    if (window->mouse_state == MouseState::left_button_down)
    {
      mouse_down_window = wnd.handle;
      mouse_down_pos    = window->cursor_pos();
    }
    else if (window->mouse_state == MouseState::left_button_up)
    {
      mouse_up_window = wnd.handle;
      mouse_up_pos    = window->cursor_pos();
    }
  }

  // update cursor hovered widget id
  if (!_hovered_widget_ids.empty())
    _prev_hovered_widget_id = _hovered_widget_ids.back();
  
  // lerp anim update
  _lerp_anim_timer.process_events();
}

void UIContext::add_vertices_indices(std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept
{
  auto render_data = _window->data();
  auto [min, max]  = bounding_rectangle;

  auto offset = _op_data.op == ShapeProperty::Operator::none ? _shape_properties_offset : _op_data.offset;

  render_data->vertices.append_range(std::vector<Vertex>
  {
    { { min.x, min.y, 0.f }, { 0.f, 0.f }, offset },
    { { max.x, min.y, 0.f }, { 1.f, 0.f }, offset },
    { { max.x, max.y, 0.f }, { 1.f, 1.f }, offset },
    { { min.x, max.y, 0.f }, { 0.f, 1.f }, offset },
  });
  render_data->indices.append_range(std::vector<uint16_t>
  {
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 1),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 3),
  });
  _idx_beg += 4;
}

void UIContext::add_shape_property(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept
{
  auto render_data = _window->data();
  render_data->shape_properties.emplace_back(ShapeProperty
  {
    type,
    _tmp_color.value_or(color),
    thickness,
    _op_data.op,
    values
  });
  _shape_properties_offset += render_data->shape_properties.back().byte_size();
}

void UIContext::add_shape(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept
{
  auto [min, max] = bounding_rectangle;

  if (_op_data.op == ShapeProperty::Operator::u)
  {
    _op_data.points.emplace_back(min);
    _op_data.points.emplace_back(max);
    goto add_shape_property;
  }

  add_vertices_indices(bounding_rectangle);
add_shape_property:
  add_shape_property(type, color, thickness, values);
}

auto UIContext::add_lerp_anim(size_t id, uint32_t dur) noexcept -> LerpAnimation*
{
  if (!_lerp_anims.contains(id))
    _lerp_anims[id].init(&_lerp_anim_timer, dur);
  return &_lerp_anims[id];
}

auto UIContext::is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom, LerpAnimation* lerp_anim) noexcept -> bool
{
  return lerp_anim->update([&]
  {
    if (ui::is_hover_on(left_top, right_bottom))
    {
      g_ui_ctx._hovered_widget_ids.push_back(id);
      return id == g_ui_ctx._prev_hovered_widget_id;
    }
    return false;
  });
}

void add_title_bar() noexcept
{ 
  auto btn_width   = Title_Bar_Button_Width;
  auto btn_height  = Title_Bar_Height;
  auto icon_width  = Title_Bar_Button_Icon_Width;
  auto icon_height = Title_Bar_Button_Icon_Height;
#if 0
  uint32_t background_colors[2] = { 0xffffffff, 0xeeeeeeff };
  auto i = is_active() || is_moving() || is_resizing();

  auto background_color = color_lerp(background_colors[0], background_colors[1], add_lerp_anim(generic_id("__update_title_bar"), 200)->update(i).get_lerp());

  auto [w, h] = window_extent();

  Tmp_Render_Pos(0, 0)
  {
    ui::rectangle({}, { w, btn_height }, background_color);
    ui::add_move_invalid_area({ 0, btn_height }, { w, h });

    // minimize button
    if (button(w - btn_width * 3, 0, btn_width, btn_height, background_color, 0x0cececeff,
      [] (uint32_t width, uint32_t height) { ui::line({ 0, height / 2 }, { width, height / 2 }); },
      icon_width, icon_height, 0x395063ff, 0x395063ff))
      minimize_window();

    // maximize / restore button
    if (button(w - btn_width * 2, 0, btn_width, btn_height, background_color, 0x0cececeff,
      [&] (uint32_t width, uint32_t height)
      {
        if (is_maxmized())
        {
          auto padding_x = width / 5;
          auto padding_y = width / 5;
          ui::rectangle({ padding_x, 0 }, { width, height - padding_y }, 0, 1);
          ui::discard_rectangle({ 0, padding_y }, { width - padding_x, height });
          ui::rectangle({ 0, padding_y }, { width - padding_x, height }, 0, 1);
        }
        else
          ui::rectangle({}, { width, height }, 0, 1);
      },
      icon_width, icon_height, 0x395063ff, 0x395063ff))
      is_maxmized() ? restore_window() : maximize_window();

    // close button
    if (button(w - btn_width, 0, btn_width, btn_height, background_color, 0xeb1123ff,
      [] (uint32_t width, uint32_t height)
      {
        ui::line({}, { width, height });
        ui::line({ width, 0 }, { 0, height });
      }, icon_width, icon_height, 0x395063ff, 0xffffffff))
      close_window();

    ui::add_move_invalid_area({ w - btn_width * 3, 0 }, { w, btn_width });
  }
#endif
}

auto UIContext::generic_id(std::string_view name) const noexcept -> size_t
{
  auto id = generic_hash(_window->handle, name);
  err_if(_ids.contains(id), "cannot duplicate id {}", name);
  return id;
}

}}
