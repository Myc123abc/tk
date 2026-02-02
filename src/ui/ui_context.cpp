#include "ui_context.hpp"
#include "util/error_handling.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/renderer.hpp"
#include "../util/hash.hpp"

#include <stb_image.h>

using namespace tk::renderer;

namespace tk { namespace ui {

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

  min -= 1;
  max += 1;

  return { min, max };
}

auto is_caps_locked() noexcept -> bool
{
  return GetKeyState(VK_CAPITAL) & 0b1;
}

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

void Window::add_move_invald_areas(RECT rect) noexcept
{
  auto idx = 1 - move_invalid_areas_idx.load(std::memory_order_relaxed);
  move_invalid_areas[idx].emplace_back(rect);
}

void Window::clear_move_invalid_areas() noexcept
{
  auto idx = 1 - move_invalid_areas_idx.load(std::memory_order_relaxed);
  move_invalid_areas[idx].clear();
}

void Window::switch_move_invalid_areas() noexcept
{
  auto idx = 1 - move_invalid_areas_idx.load(std::memory_order_relaxed);
  move_invalid_areas_idx.store(idx, std::memory_order_release);
}

auto Window::access_move_invliad_areas() noexcept -> std::vector<RECT>&
{
  return move_invalid_areas[move_invalid_areas_idx.load(std::memory_order_acquire)];
}

////////////////////////////////////////////////////////////////////////////////
///                             UIContext
////////////////////////////////////////////////////////////////////////////////

void UIContext::init() noexcept
{
  _fullscreen_window.snap = g_wnd_mgr.create_fullscreen_window();
}

void UIContext::destroy() noexcept
{
  close_window();
  g_wnd_mgr.close_fullscreen_window();
}

void UIContext::begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg) noexcept
{
  err_if(_call_begin, "begin is called but end not be called");
  _call_begin = true;

  // create window if not have
  if (!_windows.contains(name.data()))
  {
    _windows.emplace(name.data(), g_wnd_mgr.create_window(x, y, width, height));
    _window = &_windows[name.data()];
    _window_names.emplace(_window->snap.handle, name.data());
    _window->can_be_closed = is_closed;
    for (auto& handle : _window->datas) handle = g_render_data_pool.alloc();
  }

  _window = &_windows[name.data()];
  _window->is_called = true;
  _window->cfg       = cfg;
  if (is_closed)
    *is_closed = _window->is_closed;

  _shape_properties_offset = 0;
  _idx_beg                 = 0;

  if (_window->cfg.display_title_bar)
    set_render_pos(0, Title_Bar_Height);
  else
    set_render_pos(0, 0);

  _window->clear_move_invalid_areas();
  _window->data()->wait();
}

void UIContext::end() noexcept
{
  err_if(!_call_begin, "begin is not called but end is called");
  if (_window->cfg.display_title_bar)
    add_title_bar();

  // draw real window wireframe, and virtual window wireframe
#ifndef NDEBUG
  set_render_pos(-Window_Shadow_Thickness, -Window_Shadow_Thickness);
  auto extent = window_extent();
  ui::rectangle({}, { extent.x + Window_Shadow_Thickness * 2, extent.y + Window_Shadow_Thickness * 2 }, 0x00ff00ff, 1);
  set_render_pos(0, 0);
  ui::rectangle({}, extent, 0xffff00ff, 1);
#endif

  _call_begin = false;
  _window->switch_move_invalid_areas();
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

void UIContext::check_union_draw() const noexcept
{
  err_if(!_union_begin, "not called union begin");
}

void UIContext::check_union_not_draw() const noexcept
{
  err_if(_union_begin, "union begin is called");
}

void UIContext::close_window() noexcept
{
  std::erase_if(_windows, [this](auto& pair)
  {
    auto& window = pair.second;
    if (window.is_called)
    {
      window.is_called = false;
      return false;
    }
    else
    {
      g_wnd_mgr.close_window(window.snap.handle, { window.datas.begin(), window.datas.end() });
      _window_names.erase(window.snap.handle);
      return true;
    }
  });
}

void UIContext::preprocess_render() noexcept
{
  close_window();

  // TODO: wait images upload complete

}

void UIContext::render() noexcept
{
  preprocess_render();

  // acquire frame, for synchronous with present vsync
  g_renderer.acquire_frame();

  // wakeup if renderer is sleeping, which is no render data has
  auto need_wakeup = g_renderer.is_sleeping();

  // process window render datas
  static auto render = [](Window& wnd, RenderData* data)
  {
    g_renderer.render(wnd.snap.handle, data);
    if (data) wnd.next_frame();
  };
  for (auto& wnd : _windows | std::views::values)
  {
    auto data = wnd.data();
    if (!wnd.snap.resizing)
    {
      data->scissor_rect.left   = Window_Shadow_Thickness;
      data->scissor_rect.top    = Window_Shadow_Thickness;
      data->scissor_rect.right  = data->scissor_rect.left + wnd.snap.width;
      data->scissor_rect.bottom = data->scissor_rect.top  + wnd.snap.height;
      render(wnd, wnd.data());
    }
    else
    {
      if (wnd.need_clear)
      {
        wnd.need_clear = false;
        render(wnd, {});
      }
      data->scissor_rect        = wnd.rect();
      data->resizing_window_pos = wnd.real_pos();
      render(_fullscreen_window, wnd.data());
    }
  }
  if (_fullscreen_window.need_clear)
  {
    _fullscreen_window.need_clear = false;
    render(_fullscreen_window, {});
  }

  // wakeup renderer if prev frame no render data
  if (need_wakeup && !_windows.empty())
    g_renderer.wakeup();

  // process message queue
  _msg_queue.process(MessageHandler{ g_ui_ctx });

  postprocess_render();
}

void UIContext::postprocess_render() noexcept
{
  // clear state
  _ids.clear();
  if (mouse_up_window || _interrupte)
  {
    mouse_down_window     = {};
    mouse_up_window       = {};
    mouse_down_pos        = {};
    mouse_up_pos          = {};
    is_move_from_maximize = {};
    _btn_state            = {};
    _interrupte           = {};
  }

  // update cursor hovered widget id
  if (!_hovered_widget_ids.empty())
  {
    _prev_hovered_widget_id = _last_hovered_widget_id;
    _hovered_widget_ids.clear();
  }

  // update mouse state
  auto mouse_left_button_state = get_key(Key::Mouse_Left_Button);
  if (auto it = std::ranges::find_if(_windows, [](auto const& pair) { return pair.second.is_active(); }); it != _windows.end())
  {
    auto& window = it->second;
    if (mouse_left_button_state == KeyState::down)
    {
      mouse_down_window       = window.snap.handle;
      mouse_down_pos          = window.cursor_pos();
      is_move_from_maximize = window.snap.move_from_maximize;
    }
    else if (mouse_left_button_state == KeyState::up)
    {
      mouse_up_window = window.snap.handle;
      mouse_up_pos    = window.cursor_pos();
    }
  }

  // update button state
  if (_btn_state.id)
  {
    auto pos = get_cursor_pos();
    if (!point_on(get_cursor_pos(), _btn_state.left_top, _btn_state.right_bottom))
      _btn_state.move_out = true;
  }

  update_keys();

  // update delta time
  static auto tp = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  _delta_time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now - tp).count());
  tp          = now;

  // fps
  static auto acc_time = 0;
  acc_time += _delta_time;
  static auto count = 0;
  ++count;
  if (acc_time >= 1000'000)
  {
    info("fps {}", count);
    acc_time = 0;
    count    = 0;
  }
}

void UIContext::add_mouse_left_button_state(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept
{
  check_draw();
  if (!_btn_state.id)
  {
    auto pos = _window->pos();
    left_top     += pos;
    right_bottom += pos;
    _btn_state = { id, left_top, right_bottom };
  }
}

auto UIContext::is_cursor_move_out(size_t id) noexcept -> bool
{
  return _btn_state.id != id ? false : _btn_state.move_out;
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

void UIContext::render_on(int x, int y, std::move_only_function<void()>&& func) noexcept
{
  assert(func);
  auto org_pos = g_ui_ctx.get_render_pos();
  g_ui_ctx.set_render_pos(x, y);
  func();
  g_ui_ctx.set_render_pos(org_pos.x - Window_Shadow_Thickness, org_pos.y - Window_Shadow_Thickness);
}

auto UIContext::is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool
{
  if (ui::is_hover_on(left_top, right_bottom))
  {
    _hovered_widget_ids.emplace(id);
    _last_hovered_widget_id = id;
    return id == _prev_hovered_widget_id;
  }
  return false;
}

void UIContext::add_title_bar() noexcept
{
  auto btn_width   = Title_Bar_Button_Width;
  auto btn_height  = Title_Bar_Height;
  auto icon_width  = Title_Bar_Button_Icon_Width;
  auto icon_height = Title_Bar_Button_Icon_Height;

  auto is_active        = _window->is_active();
  auto value            = lerp_ping_pong(is_active, generic_id("tk::ui::update_title_bar_background_color"), 200'000);
  auto background_color = lerp(0xffffffff, 0xeeeeeeff, value);

  auto btn_mouse_down_color       = 0xb0b0b0ff;
  auto btn_hovered_color          = is_active ? 0xcececeff : 0xddddddff;
  auto close_btn_mouse_down_color = 0xea6a75ff;
  auto close_btn_hovered_color    = is_active ? 0xe81123ff : 0xe81123ff;

  auto w = _window->snap.width;
  auto h = _window->snap.height;

  set_render_pos(0, 0);

  draw_title_bar = true;

  ui::rectangle({}, { w, btn_height }, background_color);
  ui::add_move_invalid_area({ 0, btn_height }, { w, h });

  auto handle = _window->snap.handle;

  // minimize button
  if (button("tk::ui::title_bar_minimize_button", w - btn_width * 3, 0, btn_width, btn_height, background_color, btn_hovered_color, btn_mouse_down_color,
    [] (uint32_t width, uint32_t height) { ui::line({ 0, height / 2 }, { width, height / 2 }); },
    icon_width, icon_height, 0x395063ff, 0x395063ff))
    g_wnd_mgr.minimize_window(handle);

  // maximize / restore button
  if (button("tk::ui::title_bar_maximize_restore_button", w - btn_width * 2, 0, btn_width, btn_height, background_color, btn_hovered_color, btn_mouse_down_color,
    [&] (uint32_t width, uint32_t height)
    {
      if (_window->snap.maximized)
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
    _window->snap.maximized ? g_wnd_mgr.restore_window(handle) : g_wnd_mgr.maximize_window(handle);

  // close button
  if (button("tk::ui::title_bar_close_button", w - btn_width, 0, btn_width, btn_height, background_color, close_btn_hovered_color, close_btn_mouse_down_color,
    [] (uint32_t width, uint32_t height)
    {
      ui::line({}, { width, height });
      ui::line({ width, 0 }, { 0, height });
    }, icon_width, icon_height, 0x395063ff, 0xffffffff))
    _window->is_closed = true;

  ui::add_move_invalid_area({ w - btn_width * 3, 0 }, { w, btn_height });
  draw_title_bar = false;
}

auto UIContext::get_id(std::string_view name) const noexcept -> size_t
{
  return generic_hash(_window->snap.handle, name);
}

auto UIContext::generic_id(std::string_view name) noexcept -> size_t
{
  auto id = get_id(name);
  err_if(_ids.contains(id), "cannot duplicate id {}", name);
  _ids.emplace(id);
  return id;
}

auto UIContext::get_lerpolator(size_t id, double duration) noexcept -> Lerpolator*
{
  if (!_lerpolators.contains(id))
    _lerpolators[id].init(duration);
  return &_lerpolators.at(id);
}

void UIContext::remove_lerpolator(size_t id) noexcept
{
  err_if(!_lerpolators.contains(id), "remove an unexist color lerpolator");
  _lerpolators.erase(id);
}

void UIContext::reset_lerpolator(size_t id) noexcept
{
  err_if(!_lerpolators.contains(id), "remove an unexist color lerpolator");
  _lerpolators.at(id).reset();
}

auto UIContext::lerp_ping_pong(bool b, size_t id, double duration) noexcept -> double
{
  auto lerpolator = g_ui_ctx.get_lerpolator(id, duration);

  if (b)
  {
    if (!lerpolator->is_finished())
    {
      if (lerpolator->is_reversed())
        lerpolator->reverse();
      lerpolator->start();
    }
  }
  else
  {
    if (lerpolator->is_finished())
    {
      lerpolator->reverse();
      lerpolator->start();
    }
    else if (lerpolator->is_started() && !lerpolator->is_reversed())
      lerpolator->reverse();
  }

  lerpolator->update(ui::delta_time());

  auto value = lerpolator->get();

  if (lerpolator->is_finished() && lerpolator->is_reversed())
    g_ui_ctx.remove_lerpolator(id);

  return value;
}

auto UIContext::access_move_invalid_areas(HWND handle) noexcept -> std::vector<RECT>&
{
  auto& window = get_window(handle);
  return window.access_move_invliad_areas();
}

void UIContext::begin_path() noexcept
{
  check_draw();
  check_path_not_draw();
  _path_begin = true;
  path_data.push_back(std::bit_cast<float>(0u)); // record count
}

void UIContext::end_path(Color color, float thickness) noexcept
{
  check_draw();
  check_path_draw();
  err_if(path_data.empty(), "path drawing not have any data");

  add_shape(ShapeProperty::Type::path, color, thickness, path_data, get_bounding_rectangle(path_points));

  _path_begin = {};
  path_data.clear();
  path_points.clear();
}

void UIContext::begin_union() noexcept
{
  check_draw();
  check_path_not_draw();
  err_if(_union_begin, "cannot call begin union twice");
  _union_begin    = true;
  _op_data.op     = ShapeProperty::Operator::u;
  _op_data.offset = _shape_properties_offset;
}

void UIContext::end_union(Color color, float thickness) noexcept
{
  check_draw();
  err_if(!_union_begin, "cannot call end union in an uncomplete unino operator");
  err_if(_path_begin, "cannot call end union in an uncomplete path draw");
  _union_begin = false;
  auto render_data = _window->data();
  render_data->shape_properties.back().set_color(_tmp_color.value_or(color));
  render_data->shape_properties.back().set_thickness(thickness);
  render_data->shape_properties.back().set_operator({});

  add_vertices_indices(get_bounding_rectangle(_op_data.points));

  _op_data.op     = {};
  _op_data.offset = {};
  _op_data.points.clear();
}

void UIContext::update_keys() noexcept
{
  using enum KeyState;

  for (auto it = _down_keys.begin(); it != _down_keys.end();)
  {
    auto& ctx = _keys.at(*it);

    if (ctx.state == down)
    {
      ctx.state = down_idle;
      ctx.dur += delta_time();
    }
    else if (ctx.state == down_idle)
    {
      ctx.dur += delta_time();
      if (ctx.dur > Key_Repeate_Start_Duration)
      {
        ctx.dur   = {};
        ctx.state = press;
      }
    }
    
    if (ctx.state == up)
    {
      ctx.state = idle;
      it = _down_keys.erase(it);
      continue;
    }
    else if (GetAsyncKeyState(static_cast<SHORT>(*it)) == 0)
    {
      ctx.state = up;
      ctx.dur   = {};
    }

    ++it;
  }
}

auto UIContext::get_key(Key key) noexcept -> KeyState
{
  using enum KeyState;

  auto& ctx = _keys.at(key);
  if (ctx.state == idle && GetAsyncKeyState(static_cast<SHORT>(key)) < 0)
  {
    assert(!_down_keys.contains(key));
    ctx.state = down;
    _down_keys.emplace(key);
  }

  return ctx.state;
}

void UIContext::image(std::string_view path, glm::vec2 pos) noexcept
{
  check_draw();
  check_path_not_draw();
  check_union_not_draw();

  if (_images.contains(path.data()))
  {
    auto const& img = _images.at(path.data());
    pos += get_render_pos();
    add_shape(ShapeProperty::Type::image, {}, {}, { std::bit_cast<float>(img.index) }, { { pos.x, pos.y }, { pos.x + img.width, pos.y + img.height }});
    return;
  }

  int  w, h, ch;
  auto data = stbi_load(path.data(), &w, &h, &ch, 4);
  if (!data)
  {
    warn("not found image {}", path);
    return;
  }

  auto ptr = reinterpret_cast<Renderer::UploadImageInfo*>(malloc(sizeof(Renderer::UploadImageInfo)));
  ptr->bitmap.init(w, h, 4, data);
  ptr->event = CreateEventW(nullptr, false, false, nullptr);
  g_renderer.send_message(Renderer::Message_Upload_Image{ ptr });
  g_renderer.wakeup();
  WaitForSingleObject(ptr->event, INFINITE);
  CloseHandle(ptr->event);

  pos += get_render_pos();
  add_shape(ShapeProperty::Type::image, {}, {}, { std::bit_cast<float>(ptr->index) }, { { pos.x, pos.y }, { pos.x + w, pos.y + h }});
  _images.emplace(path, ImageInfo{ ptr->bitmap.width, ptr->bitmap.height, 4u, ptr->index });

  free(ptr);
}

}}
