#include "ui_context.hpp"
#include "util/error_handling.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/renderer.hpp"
#include "../util/hash.hpp"
#include "text_engine/text_engine.hpp"

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
  _fullscreen_window = g_wnd_mgr.create_fullscreen_window();
  g_text_engine.init();
  FrameData::init();
}

void UIContext::destroy() noexcept
{
  g_text_engine.destroy();
  close_window();
  g_wnd_mgr.close_fullscreen_window();
}

void UIContext::begin(std::string_view name, int x, int y, uint width, uint height, bool* is_closed, WindowConfig const& cfg) noexcept
{
  err_if(_call_begin, "begin is called but end not be called");
  _call_begin = true;

  // create window if not have
  if (!_wnd_ctxs.contains(name.data()))
  {
    auto handle = g_wnd_mgr.create_window(x, y, width, height, cfg.backdrop);
    _wnd_ctxs.emplace(name.data(), handle);
    _wnd_names.emplace(handle, name);
    _wnd_ctx = &_wnd_ctxs[name.data()];
    _wnd_ctx->can_be_closed = is_closed;
    _wnd = g_wnd_mgr.get_window(_wnd_ctx->handle);
  }
  else 
  {
    _wnd_ctx = &_wnd_ctxs[name.data()];
    _wnd = g_wnd_mgr.get_window(_wnd_ctx->handle);
  }
  _wnd_ctx->is_called = true;

  update_window_config(cfg);

  if (is_closed)
    *is_closed = _wnd_ctx->is_closed;

  // fullscreen process
  fullscreen_process();  

  if (is_use_title_bar_now())
    set_render_pos(0, Title_Bar_Height);
  else
    set_render_pos(0, 0);

  _wnd->clear_move_invalid_areas();
  _wnd_ctx->frame_data.clear();
  if (_wnd->is_fullscreen())
    _wnd->add_move_invalid_area({ 0, 0, static_cast<LONG>(_wnd->width()), static_cast<LONG>(_wnd->height()) });
}

void UIContext::update_window_config(WindowConfig const& cfg) noexcept
{
  auto& wnd_cfg = _wnd->cfg();
  if (cfg.backdrop.style != ui::BackdropStyle::none)
  {
    if (_wnd_ctx->first_time_call) goto label_end;
    if (wnd_cfg.backdrop.style == ui::BackdropStyle::none)
      g_wnd_mgr.init_blur_window(_wnd_ctx->handle, cfg.backdrop);
    else
      g_wnd_mgr.update_blur_window(_wnd_ctx->handle, cfg.backdrop);
  }
  else
  {
    if (wnd_cfg.backdrop.style != ui::BackdropStyle::none)
      g_wnd_mgr.remove_blur_window(_wnd_ctx->handle);
  }
label_end:
  wnd_cfg = cfg;
}

void UIContext::fullscreen_process() noexcept
{
  if (_wnd_ctx->set_fullscreen)
  {
    // fullscreen window
    if (!_wnd->is_fullscreen())
      _wnd->fullscreen();
  }
  else
  {
    // restore window
    if (_wnd->is_fullscreen())
      _wnd->restore_fullscreen();
  }
}

void UIContext::end() noexcept
{
  err_if(!_call_begin, "begin is not called but end is called");
  if (is_use_title_bar_now())
    add_title_bar();
  _call_begin = false;
  if (_wnd_ctx->first_time_call) _wnd_ctx->first_time_call = false;
}

void UIContext::check_draw() const noexcept
{
  err_if(!_call_begin, "not called begin to draw something");
}

void UIContext::close_window() noexcept
{
  std::erase_if(_wnd_ctxs, [this](auto& pair)
  {
    auto& ctx = pair.second;
    if (ctx.is_called)
    {
      ctx.is_called = false;
      return false;
    }
    else
    {
      _wnd_names.erase(ctx.handle);
      g_wnd_mgr.close_window(ctx.handle);
      cursor_on_window = {};
      return true;
    }
  });
}

void UIContext::fullscreen_window() noexcept
{
  if (_wnd_ctx->set_fullscreen || _wnd->is_moving_or_resizing()) return;
  _wnd_ctx->set_fullscreen = true;
}

void UIContext::restore_fullscreen_window() noexcept
{
  if (!_wnd_ctx->set_fullscreen) return;
  _wnd_ctx->set_fullscreen = false;
}

void UIContext::preprocess_render() noexcept
{
  close_window();
  g_text_engine.upload_uncached_glyphs();
}

void UIContext::render() noexcept
{
  preprocess_render();

  // process window render datas
  for (auto& wnd_ctx : _wnd_ctxs | std::views::values)
  {
    auto handle = wnd_ctx.handle;
    auto data   = &wnd_ctx.frame_data;

    _wnd = g_wnd_mgr.get_window(handle);

    if (!_wnd->is_resizing())
    {
      data->add_scissor_rect(_wnd->content_rect());
      if (!_wnd->is_fullscreen() && !_wnd->is_maximized())
        window_shadow_wireframe_process(wnd_ctx, *_wnd, _wnd->shadow_rect());
      data->build_render_cmds();
      g_renderer.submit({ handle, data });
    }
    else
    { 
      if (wnd_ctx.need_clear)
      {
        wnd_ctx.need_clear = false;
        g_renderer.submit({ handle });
      }
      data->set_window_pos(_wnd->real_pos());
      data->add_scissor_rect(_wnd->rect());
      window_shadow_wireframe_process(wnd_ctx, *_wnd, _wnd->real_rect());
      data->build_render_cmds();
      if (_wnd->cfg().backdrop.style != ui::BackdropStyle::none)
        g_renderer.submit({ _fullscreen_window, data, handle, _wnd->rect() });
      else
        g_renderer.submit({ _fullscreen_window, data });
    }
  }
  if (_fullscreen_window_need_clear)
  {
    _fullscreen_window_need_clear = false;
    g_renderer.submit({ _fullscreen_window });
  }

  postprocess_render();
}

void UIContext::clear_state() noexcept
{
  mouse_down_window     = {};
  mouse_up_window       = {};
  mouse_down_pos        = {};
  mouse_up_pos          = {};
  is_move_from_maximize = {};
  _btn_state            = {};
}

void UIContext::postprocess_render() noexcept
{
  // clear state
  _ids.clear();
  if (mouse_up_window) clear_state();

  // update cursor hovered widget id
  if (!_hovered_widget_ids.empty())
  {
    _prev_hovered_widget_id = _last_hovered_widget_id;
    _hovered_widget_ids.clear();
  }

  // update mouse state
  auto mouse_left_button_state = get_key(Key::Mouse_Left_Button);
  auto wnds                    = g_wnd_mgr.windows_view();
  if (cursor_on_window)
  {
    auto wnd = g_wnd_mgr.get_window(cursor_on_window);
    if (mouse_left_button_state == KeyState::down)
    {
      mouse_down_window = wnd->handle();
      mouse_down_pos    = wnd->cursor_pos();
      if (!is_move_from_maximize)
        is_move_from_maximize = wnd->is_move_from_maximize();
    }
    else if (mouse_left_button_state == KeyState::up)
    {
      mouse_up_window = wnd->handle();
      mouse_up_pos    = wnd->cursor_pos();
    }
  };

  // update button state
  if (_btn_state.id)
  {
    auto pos = get_cursor_pos();
    if (!Rect{ _btn_state.left_top, _btn_state.right_bottom }.contains(get_cursor_pos()))
      _btn_state.move_out = true;
  }

  update_keys();

  // update delta time
  static auto tp = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  _delta_time = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now - tp).count());
  tp          = now;
}

void UIContext::window_shadow_wireframe_process(WindowContext& wnd_ctx, renderer::Window const& wnd, Rect scissor_rect) noexcept
{
  auto const& cfg = wnd.cfg();
  if (!cfg.display_window_shadow && !cfg.wireframe_color)
    return;

  auto data = &wnd_ctx.frame_data;
  auto col  = float4{};

  auto get_wireframe_color = [&] -> std::optional<float4>
  {
    if (cfg.wireframe_color)
    {
      auto tmp_wnd_ctx = _wnd_ctx;
      _wnd_ctx = &wnd_ctx;

      auto color = cfg.wireframe_color.value();
      if (cfg.display_wireframe_only_active)
      {
        auto ratio = ping_pong(wnd.is_active(), generic_id("tk::ui::render::draw_wire_frame"), Window_Active_Response_Time);
        color.a = cfg.wireframe_color.value().a * ratio;
      }

      _wnd_ctx = tmp_wnd_ctx;

      return color;
    }
    return {};
  };

  data->set_window_shadow(scissor_rect, wnd.real_extent(), wnd.shadow_thickness(), {}, cfg.display_window_shadow ? 5 : 0, 15, get_wireframe_color());
}

void UIContext::add_mouse_left_button_state(size_t id, float2 left_top, float2 right_bottom) noexcept
{
  check_draw();
  if (!_btn_state.id)
  {
    auto pos = _wnd->pos();
    left_top     += pos;
    right_bottom += pos;
    _btn_state = { id, left_top, right_bottom };
  }
}

auto UIContext::is_cursor_move_out(size_t id) noexcept -> bool
{
  return _btn_state.id != id ? false : _btn_state.move_out;
}

void UIContext::render_on(float x, float y, std::move_only_function<void()>&& func) noexcept
{
  assert(func);
  auto org_pos = g_ui_ctx.get_render_pos();
  g_ui_ctx.set_render_pos(x, y);
  func();
  g_ui_ctx.set_render_pos(org_pos.x - Window_Shadow_Thickness, org_pos.y - Window_Shadow_Thickness);
}

auto UIContext::is_hover_on(size_t id, float2 left_top, float2 right_bottom) noexcept -> bool
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

  auto is_active        = _wnd->is_active();
  auto value            = ping_pong(is_active, generic_id("tk::ui::update_title_bar_background_color"), Window_Active_Response_Time);
  auto background_color = lerp(0xffffffff, 0xeeeeeeff, value);

  auto btn_mouse_down_color       = 0xb0b0b0ff;
  auto btn_hovered_color          = is_active ? 0xcececeff : 0xddddddff;
  auto close_btn_mouse_down_color = 0xea6a75ff;
  auto close_btn_hovered_color    = is_active ? 0xe81123ff : 0xe81123ff;

  auto scale = _wnd->scale();
  auto w     = _wnd->width()  / scale;
  auto h     = _wnd->height() / scale;

  set_render_pos(0, 0);

  draw_title_bar = true;

  ui::rectangle({}, { w, btn_height }, background_color);
  ui::add_move_invalid_area({ 0, btn_height }, { w, h });

  auto handle = _wnd->handle();

  // minimize button
  if (button("tk::ui::title_bar_minimize_button", w - btn_width * 3, 0, btn_width, btn_height, background_color, btn_hovered_color, btn_mouse_down_color,
    [] (uint width, uint height, Color col) { ui::line({ 0, height / 2 }, { width, height / 2 }, col, 1); },
    icon_width, icon_height, 0x395063ff, 0x395063ff))
    _wnd->minimize();

  // maximize / restore button
  if (button("tk::ui::title_bar_maximize_restore_button", w - btn_width * 2, 0, btn_width, btn_height, background_color, btn_hovered_color, btn_mouse_down_color,
    [&] (uint width, uint height, Color col)
    {
      if (_wnd->is_maximized())
      {
        auto padding_x = width / 5;
        auto padding_y = width / 5;
        ui::discard_beg([=] { ui::rectangle({ 0, padding_y }, { width - padding_x, height }); });
        ui::rectangle({ padding_x, 0 }, { width, height - padding_y }, col, 1);
        ui::discard_end();
        ui::rectangle({ 0, padding_y }, { width - padding_x, height }, col, 1);
      }
      else
        ui::rectangle({}, { width, height }, col, 1);
    },
    icon_width, icon_height, 0x395063ff, 0x395063ff) && !_wnd->cfg().no_resize)
      _wnd->is_maximized() ? _wnd->restore() : _wnd->maximize();

  // close button
  if (button("tk::ui::title_bar_close_button", w - btn_width, 0, btn_width, btn_height, background_color, close_btn_hovered_color, close_btn_mouse_down_color,
    [] (uint width, uint height, Color col)
    {
      ui::line({}, { width, height }, col);
      ui::line({ width, 0 }, { 0, height }, col);
    }, icon_width, icon_height, 0x395063ff, 0xffffffff))
    _wnd_ctx->is_closed = true;

  ui::add_move_invalid_area({ w - btn_width * 3, 0 }, { w, btn_height });
  draw_title_bar = false;
}

auto UIContext::get_id(std::string_view name) const noexcept -> size_t
{
  return generic_hash(_wnd_ctx->handle, name);
}

auto UIContext::generic_id(std::string_view name) noexcept -> size_t
{
  auto id = get_id(name);
  err_if(_ids.contains(id), "cannot duplicate id {}", name);
  _ids.emplace(id);
  return id;
}

auto UIContext::get_tween(size_t id, double duration, Tween::Ease ease) noexcept -> Tween*
{
  if (!_tweens.contains(id))
    _tweens[id].init(duration, {}, ease);
  return &_tweens[id];
}

void UIContext::remove_tween(size_t id) noexcept
{
  err_if(!_tweens.contains(id), "remove an unexist color tween");
  _tweens.erase(id);
}

void UIContext::reset_tween(size_t id) noexcept
{
  if (_tweens.contains(id)) _tweens[id].reset();
}

auto UIContext::ping_pong(bool b, size_t id, double duration, Tween::Ease ease) noexcept -> double
{
  auto tween = g_ui_ctx.get_tween(id, duration, ease);

  if (b)
  {
    if (!tween->is_finished())
    {
      if (tween->is_reversed())
        tween->reverse();
      tween->start();
    }
  }
  else
  {
    if (tween->is_finished())
    {
      tween->reverse();
      tween->start();
    }
    else if (tween->is_started() && !tween->is_reversed())
      tween->reverse();
  }

  tween->update(ui::delta_time());

  auto value = tween->get();

  if (tween->is_finished() && tween->is_reversed())
    g_ui_ctx.remove_tween(id);

  return value;
}

void UIContext::update_keys() noexcept
{
  using enum KeyState;

  for (auto it = _down_keys.begin(); it != _down_keys.end();)
  {
    auto& ctx = _keys[*it];

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

auto UIContext::get_key(Key key) noexcept -> Flag<KeyState>
{
  using enum KeyState;

  auto& ctx = _keys[key];
  if (ctx.state == idle && GetAsyncKeyState(static_cast<SHORT>(key)) < 0)
  {
    assert(!_down_keys.contains(key));
    ctx.state = down;
    _down_keys.emplace(key);
  }

  return ctx.state;
}

auto UIContext::image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha, std::optional<ImageConfig> cfg) noexcept -> std::expected<void, ImageLoadError::Type>
{
  check_draw();

  auto res = g_img_mgr.try_load(path, right_bottom.x - left_top.x, right_bottom.y - left_top.y);
  if (res)
  {
    auto img = res.value();
    auto uvs = std::vector<float2>{ { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 }, };
    if (cfg)
    {
      if (cfg->cfg.is<ImageConfig::Blur>())
      {
        auto ext  = right_bottom - left_top;
        auto blur = cfg->cfg.get<ImageConfig::Blur>();
        img = g_img_mgr.blur(img, ext, blur.sigma, blur.cnt);
        uvs[2]   = ext / g_img_mgr[img].extent();
        uvs[1].x = uvs[2].x;
        uvs[3].y = uvs[2].y;
      }
    }
    frame_data()->add_image(img, left_top, right_bottom, alpha, uvs);
    return {};
  }
  return std::unexpected(res.error());
}

auto UIContext::text(std::string_view text, float2 pos, float size, Color inner_color, FontStyle style, Color outer_color) noexcept -> TextResult
{
  if (text.empty()) return {};

  check_draw();

  auto result_handle = g_text_engine.parse(text, style);
  frame_data()->add_text(result_handle, pos, size, inner_color, outer_color);

  auto const& result = g_text_engine.get_parse_result(result_handle);

  return
  {
    {
      std::ranges::fold_left(result.advances, 0, [](auto res, auto v) { return res + v.x; }),
      result.max_height,
    },
    result.max_ascender
  };
}

}
