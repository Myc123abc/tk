#include "ui_context.hpp"
#include "util/error_handling.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/renderer/renderer.hpp"
#include "../util/hash.hpp"
#include "image_manager.hpp"
#include "text_engine.hpp"

using namespace tk::renderer;

namespace tk::ui {

auto is_caps_locked() noexcept -> bool
{
  return GetKeyState(VK_CAPITAL) & 0b1;
}

////////////////////////////////////////////////////////////////////////////////
///                             UIContext
////////////////////////////////////////////////////////////////////////////////

void UIContext::init() noexcept
{
  _fullscreen_window.snap = g_wnd_mgr.create_fullscreen_window();
  g_text_engine.init();
}

void UIContext::destroy() noexcept
{
  g_text_engine.destroy();
  g_img_mgr.destroy();
  close_window();
  g_wnd_mgr.close_fullscreen_window();
}

void UIContext::begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig const& cfg) noexcept
{
  err_if(_call_begin, "begin is called but end not be called");
  _call_begin = true;

  // create window if not have
  if (!_windows.contains(name.data()))
  {
    _windows.emplace(name.data(), g_wnd_mgr.create_window(x, y, width, height, cfg.blur_backdrop));
    _window = &_windows[name.data()];
    _window_names.emplace(_window->snap.handle, name.data());
    _window->can_be_closed = is_closed;
  }

  _window = &_windows[name.data()];
  _window->is_called = true;

  update_window_config(cfg);

  if (is_closed)
    *is_closed = _window->is_closed;

  // fullscreen process
  fullscreen_process();  

  if (is_use_title_bar_now())
    set_render_pos(0, Title_Bar_Height);
  else
    set_render_pos(0, 0);

  _window->move_invalid_areas.data().clear();
  _window->cmd()->wait().reset();

  if (_window->snap.fullscreen_window)
    g_ui_ctx._window->add_move_invald_areas(
    {
      0,
      0,
      static_cast<LONG>(_window->snap.width),
      static_cast<LONG>(_window->snap.height)
    });
  
  _window->cfgs.data() = cfg;
}

void UIContext::update_window_config(WindowConfig const& cfg) noexcept
{
  if (cfg.blur_backdrop.enable)
  {
    if (!_window->cfg.blur_backdrop.enable)
      g_wnd_mgr.init_blur_window(_window->snap.handle, cfg.blur_backdrop);
    else if (cfg.blur_backdrop.blur_radius != _window->cfg.blur_backdrop.blur_radius)
      g_wnd_mgr.update_blur_window(_window->snap.handle, cfg.blur_backdrop);
  }
  else
  {
    if (_window->cfg.blur_backdrop.enable)
      g_wnd_mgr.remove_blur_window(_window->snap.handle);
  }
  _window->cfg = cfg;
}

void UIContext::fullscreen_process() noexcept
{
  if (_window->set_fullscreen)
  {
    // fullscreen window
    if (!_window->snap.fullscreen_window)
      g_wnd_mgr.fullscreen_window(_window->snap.handle);
  }
  else
  {
    // restore window
    if (_window->snap.fullscreen_window)
      g_wnd_mgr.restore_fullscreen_window(_window->snap.handle);
  }
}

void UIContext::end() noexcept
{
  err_if(!_call_begin, "begin is not called but end is called");
  if (is_use_title_bar_now())
    add_title_bar();
  _call_begin = false;
  _window->move_invalid_areas.swap();
  _window->cfgs.swap();
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
      for (auto& cmd : window.cmds) cmd.wait();
      g_wnd_mgr.close_window(window.snap.handle);
      _window_names.erase(window.snap.handle);
      return true;
    }
  });
}

void UIContext::fullscreen_window() noexcept
{
  if (_window->set_fullscreen || _window->is_moving_or_resizing()) return;
  _window->set_fullscreen = true;
}

void UIContext::restore_fullscreen_window() noexcept
{
  if (!_window->set_fullscreen) return;
  _window->set_fullscreen = false;
}

void UIContext::preprocess_render() noexcept
{
  close_window();
}

void UIContext::render() noexcept
{
  preprocess_render();

  // acquire frame, for synchronous with present vsync
  g_renderer.acquire_frame();

  // wakeup if renderer is sleeping, which is mean no commands submit
  auto need_wakeup = g_renderer.is_sleeping();

  // process window render datas
  auto render = [](Window& wnd, CommandList* cmd, HWND blur_host_window = {}, RECT blur_window_rect = {}) noexcept
  {
    g_renderer.submit({ wnd.snap.handle, cmd, blur_host_window, blur_window_rect});
    if (cmd) wnd.next_frame();
  };
  for (auto& wnd : _windows | std::views::values)
  {
    auto cmd = wnd.cmd();

    if (!wnd.snap.resizing)
    {
      auto thickness = wnd.shadow_thickness();
      cmd->set_scissor_rect({ thickness, thickness,
        static_cast<LONG>(thickness + wnd.snap.width),
        static_cast<LONG>(thickness + wnd.snap.height)});
      if (!wnd.snap.fullscreen_window && !wnd.snap.maximized)
        window_shadow_wireframe_process(wnd, { 0, 0,
          static_cast<LONG>(thickness * 2 + wnd.snap.width),
          static_cast<LONG>(thickness * 2 + wnd.snap.height)});
      cmd->submit();
      render(wnd, cmd);
    }
    else
    { 
      if (wnd.need_clear)
      {
        wnd.need_clear = false;
        render(wnd, {});
      }
      cmd->set_scissor_rect(wnd.rect());
      window_shadow_wireframe_process(wnd, wnd.real_rect());
      cmd->set_window_pos(wnd.real_pos());
      cmd->submit();
      if (wnd.cfg.blur_backdrop.enable)
        render(_fullscreen_window, cmd, wnd.snap.handle, wnd.rect());
      else
        render(_fullscreen_window, cmd);
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
      mouse_down_window = window.snap.handle;
      mouse_down_pos    = window.cursor_pos();
      if (!is_move_from_maximize)
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

void UIContext::window_shadow_wireframe_process(Window& wnd, RECT scissor_rect) noexcept
{
  if (!wnd.cfg.display_window_shadow && !wnd.cfg.wireframe_color)
    return;

  auto cmd = wnd.cmd();
  auto col = glm::vec4{};

  auto get_wireframe_color = [&] -> std::optional<glm::vec4>
  {
    if (wnd.cfg.wireframe_color)
    {
      auto old_window = _window;
      _window = &wnd;

      auto color = wnd.cfg.wireframe_color.value();
      if (wnd.cfg.display_wireframe_only_active)
      {
        auto ratio = lerp_ping_pong(wnd.is_active(), generic_id("tk::ui::render::draw_wire_frame"), Window_Active_Response_Time);
        color.a = wnd.cfg.wireframe_color.value().a * ratio;
      }

      _window = old_window;

      return color;
    }
    return {};
  };

  cmd->draw_window_shadow(wnd.extent(), wnd.shadow_thickness(), {}, wnd.cfg.display_window_shadow ? 5 : 0, 15, get_wireframe_color());
  cmd->set_scissor_rect(scissor_rect);
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
  auto value            = lerp_ping_pong(is_active, generic_id("tk::ui::update_title_bar_background_color"), Window_Active_Response_Time);
  auto background_color = lerp(0xffffffff, 0xeeeeeeff, value);

  auto btn_mouse_down_color       = 0xb0b0b0ff;
  auto btn_hovered_color          = is_active ? 0xcececeff : 0xddddddff;
  auto close_btn_mouse_down_color = 0xea6a75ff;
  auto close_btn_hovered_color    = is_active ? 0xe81123ff : 0xe81123ff;

  auto scale = get_scale();
  auto w     = _window->snap.width  / scale;
  auto h     = _window->snap.height / scale;

  set_render_pos(0, 0);

  draw_title_bar = true;

  ui::rectangle({}, { w, btn_height }, background_color);
  ui::add_move_invalid_area({ 0, btn_height }, { w, h });

  auto handle = _window->snap.handle;

  // minimize button
  if (button("tk::ui::title_bar_minimize_button", w - btn_width * 3, 0, btn_width, btn_height, background_color, btn_hovered_color, btn_mouse_down_color,
    [] (uint32_t width, uint32_t height) { ui::line({ 0, height / 2 }, { width, height / 2 }, 1); },
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
    icon_width, icon_height, 0x395063ff, 0x395063ff) && !_window->cfg.no_resize)
      _window->snap.maximized ? g_wnd_mgr.restore_window(handle) : g_wnd_mgr.maximize_window(handle);

  // close button
  if (button("tk::ui::title_bar_close_button", w - btn_width, 0, btn_width, btn_height, background_color, close_btn_hovered_color, close_btn_mouse_down_color,
    [] (uint32_t width, uint32_t height)
    {
      ui::line({}, { width, height }, 1);
      ui::line({ width, 0 }, { 0, height }, 1);
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

void UIContext::begin_path() noexcept
{
  check_draw();
  check_path_not_draw();
  _path_begin = true;
  cmd()->begin_path();
}

void UIContext::end_path(Color color, float thickness) noexcept
{
  check_draw();
  check_path_draw();
  _path_begin = false;
  cmd()->end_path(color, thickness);
}

void UIContext::begin_union() noexcept
{
  check_draw();
  check_path_not_draw();
  check_union_not_draw();
  _union_begin = true;
  cmd()->begin_union();
}

void UIContext::end_union(Color color, float thickness) noexcept
{
  check_draw();
  check_path_not_draw();
  check_union_draw();
  _union_begin = false;
  cmd()->end_union(color, thickness);
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

void UIContext::image(std::string_view path, glm::vec2 left_top, glm::vec2 right_bottom, uint8_t alpha) noexcept
{
  check_draw();
  check_path_not_draw();
  check_union_not_draw();

  if (g_img_mgr.try_load(path, right_bottom - left_top))
  {
    auto offset = get_render_pos();
    left_top     += offset;
    right_bottom += offset;
    
    auto scale = get_scale();
    left_top     *= scale;
    right_bottom *= scale;

    cmd()->image(g_img_mgr.handle(path), left_top, right_bottom, alpha);
  }
  else
    warn("image {} is not exist", path);
}

auto UIContext::text(std::string_view text, glm::vec2 pos, float size, Color inner_color, FontStyle style, Color outer_color) noexcept -> glm::vec2
{
  if (text.empty()) return {};

  check_draw();
  check_path_not_draw();
  check_union_not_draw();

  auto res = g_text_engine.parse(text);

  // text use single glyph shape property and multiple glyph boxs which share the shape property
  auto values = std::vector<float>{};
  values.emplace_back(res.glyph_atlas_index);
  values.emplace_back(inner_color.r);
  values.emplace_back(inner_color.g);
  values.emplace_back(inner_color.b);
  values.emplace_back(inner_color.a);
  values.emplace_back(outer_color.r);
  values.emplace_back(outer_color.g);
  values.emplace_back(outer_color.b);
  values.emplace_back(outer_color.a);
  values.emplace_back(.05f); // outline width, TODO: can be set by user
  // add_shape_property(ShapeProperty::Type::glyph, {}, {}, values);

  return res.extent;
}

}
