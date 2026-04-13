#pragma once

#include "frame_data.hpp"
#include "ui/ui.hpp"
#include "ui/lerpolator.hpp"
#include "../util/singleton.hpp"
#include "../renderer/window/window.hpp"

#include <windows.h>

#include <unordered_map>
#include <string>
#include <optional>
#include <unordered_set>

namespace tk::ui {

auto is_hover_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;
auto is_caps_locked() noexcept -> bool;

struct WindowContext
{
  WindowContext(HWND handle) noexcept : handle(handle) {}

  HWND      handle{};
  bool      is_called{};
  bool      can_be_closed{};
  bool      is_closed{};
  glm::vec2 render_pos{};
  bool      need_clear{};
  FrameData frame_data;
  bool      set_fullscreen{};
  bool      first_time_call{ true };
};

#define KEY_ENTRY_INIT(name, _) { Key::name, {} },

Singleton(UIContext, g_ui_ctx,
public:
  void init() noexcept;
  void destroy() noexcept;

  void close_window() noexcept;

  void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig const& cfg) noexcept;
  void end() noexcept;

  void check_draw() const noexcept;
  void check_path_draw() const noexcept;
  void check_path_not_draw() const noexcept;
  void check_union_draw() const noexcept;
  void check_union_not_draw() const noexcept;

  void render() noexcept;

  void begin_path() noexcept;
  void end_path(Color color, float thickness) noexcept;
  void begin_union() noexcept;
  void end_union(Color color, float thickness) noexcept;

  auto is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

  void set_render_pos(int x, int y) noexcept { _wnd_ctx->render_pos = { x + renderer::Window_Shadow_Thickness, y + renderer::Window_Shadow_Thickness }; }
  auto get_render_pos() const noexcept { return _wnd_ctx->render_pos; }

  void render_on(int x, int y, std::move_only_function<void()>&& func) noexcept;

  auto generic_id(std::string_view name) noexcept -> size_t;
  auto get_id(std::string_view name) const noexcept -> size_t;

  auto delta_time() const noexcept { return _delta_time; }

  auto get_lerpolator(size_t id, double duration) noexcept -> Lerpolator*;
  void remove_lerpolator(size_t id) noexcept;
  void reset_lerpolator(size_t id) noexcept;
  auto lerp_ping_pong(bool b, size_t id, double duration) noexcept -> double;

  void image(std::string_view path, glm::vec2 left_top, glm::vec2 right_bottom, uint8_t alpha) noexcept;
  auto text(std::string_view text, glm::vec2 pos, float size, Color inner_color, FontStyle style, Color outer_color) noexcept -> glm::vec2;

  void fullscreen_window() noexcept;
  void restore_fullscreen_window() noexcept;

  auto is_use_title_bar_now() const noexcept { return _wnd->cfg().display_title_bar && !_wnd->is_fullscreen(); }

  auto get_window_context(HWND handle) noexcept { assert(_wnd_names.contains(handle)); return &_wnd_ctxs[_wnd_names[handle]]; }
  void clear_fullscreen_window() noexcept { _fullscreen_window_need_clear = true; }
  void close_window(HWND handle) noexcept { get_window_context(handle)->is_closed = true; }
  void clear_state() noexcept;

private:
  void update_window_config(WindowConfig const& cfg) noexcept;

  void preprocess_render() noexcept;
  void postprocess_render() noexcept;

  void add_title_bar() noexcept;
  void fullscreen_process() noexcept;
  void window_shadow_wireframe_process(WindowContext& wnd_ctx, renderer::Window const& wnd, RECT scissor_rect) noexcept;

private:
  std::unordered_map<std::string, WindowContext> _wnd_ctxs;
  std::unordered_map<HWND, std::string>          _wnd_names;

  renderer::Window* _wnd{};
  WindowContext*    _wnd_ctx{};
  HWND              _fullscreen_window{};
  bool              _fullscreen_window_need_clear{};

  bool   _call_begin{};
  bool   _path_begin{};
  bool   _union_begin{};
  double _delta_time{};

public:
  //
  // widget ids
  //
  std::unordered_set<size_t>             _hovered_widget_ids;
  size_t                                 _last_hovered_widget_id{};
  size_t                                 _prev_hovered_widget_id{};
  std::unordered_map<size_t, Lerpolator> _lerpolators;
  std::unordered_set<size_t>             _ids;

  //
  // mouse state
  //
public:
  HWND                            cursor_on_window{};
  HWND                            mouse_down_window{};
  HWND                            mouse_up_window{};
  std::optional<glm::vec<2, int>> mouse_down_pos;
  std::optional<glm::vec<2, int>> mouse_up_pos;
  bool                            is_move_from_maximize{};
  bool                            draw_title_bar{};

  //
  // button state
  //
private:
  struct ButtonState
  {
    size_t    id{};
    glm::vec2 left_top{};
    glm::vec2 right_bottom{};
    bool      move_out{};
  } _btn_state;
  bool _interrupte{};
public:
  void add_mouse_left_button_state(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept;
  auto is_cursor_move_out(size_t id) noexcept -> bool;

  //
  // key state
  //
private:
  void update_keys() noexcept;

  struct KeyContext
  {
    KeyState state{};
    double   dur{};
  };
  std::unordered_map<Key, KeyContext> _keys
  {
    KEY_LIST(KEY_ENTRY_INIT)
  };
  std::unordered_set<Key> _down_keys;

public:
  auto get_key(Key key) noexcept -> KeyState;
)

}
