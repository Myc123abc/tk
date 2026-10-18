#pragma once

#include "frame_data/frame_data.hpp"
#include "ui/ui.hpp"
#include "ui/tween.hpp"
#include "../util/singleton.hpp"
#include "../renderer/window/window.hpp"
#include "ui/text_layout.hpp"

#include <windows.h>

#include <unordered_map>
#include <optional>
#include <unordered_set>

namespace tk::ui {

auto is_hover_on(Rect rect) noexcept -> bool;
inline auto is_hover_on(float2 left_top, float2 right_bottom) noexcept { return is_hover_on({ left_top, right_bottom }); }

auto is_caps_locked() noexcept -> bool;

struct WindowContext
{
  WindowContext() = default;
  WindowContext(HWND handle) noexcept : handle(handle) {}

  HWND      handle{};
  bool      is_called{};
  bool      can_be_closed{};
  bool      is_closed{};
  float2    render_pos{};
  bool      need_clear{};
  bool      set_fullscreen{};
  bool      first_time_call{ true };
  FrameData frame_data;
};

Singleton(UIContext, g_ui_ctx,
  friend struct WindowContext;
public:
  void init() noexcept;
  void destroy() noexcept;

  void close_window() noexcept;

  void begin(StringID name, int x, int y, uint width, uint height, bool* is_closed, WindowConfig const& cfg) noexcept;
  void end() noexcept;

  void check_draw() const noexcept;

  void render() noexcept;

  auto is_hover_on(size_t id, Rect rc) noexcept -> bool;
  auto is_hover_on(size_t id, float2 left_top, float2 right_bottom) noexcept { return is_hover_on(id, { left_top, right_bottom }); }
  auto transform_widget_rect(Rect rect) const noexcept -> Rect;

  void set_render_pos(float x, float y) noexcept { _wnd_ctx->render_pos = { x + renderer::Window_Shadow_Thickness, y + renderer::Window_Shadow_Thickness }; }
  auto get_render_pos() const noexcept { return _wnd_ctx->render_pos; }

  void render_on(float x, float y, std::move_only_function<void()>&& func) noexcept;

  auto generic_id(StringID name) noexcept -> size_t;
  auto get_id(StringID name) const noexcept -> size_t;

  auto delta_time() const noexcept { return _delta_time; }

  auto get_tween(size_t id, double forward_dur, double reverse_dur, Tween::Ease ease = {}) noexcept -> Tween*;
  auto get_tween(size_t id, double dur, Tween::Ease ease = {}) noexcept { return get_tween(id, dur, dur, ease); }
  void remove_tween(size_t id) noexcept;
  void reset_tween(size_t id) noexcept;
  auto ping_pong(bool b, size_t id, double forward_dur, double reverse_dur, Tween::Ease ease = {}) noexcept -> double;
  auto ping_pong(bool b, size_t id, double dur, Tween::Ease ease = {}) noexcept { return ping_pong(b, id, dur, dur, ease); }

  auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha, std::optional<ImageConfig> cfg) noexcept -> std::expected<void, ImageLoadErrorType>;
  auto text(StringLiteral text, float2 pos, float size, Color inner_color, TextConfig const& cfg) noexcept -> TextResult;
  auto get_text_bounding_rect(StringLiteral text, float size, TextConfig const& cfg) noexcept -> std::optional<Rect>;
  void render(TextLayout const& layout) noexcept;

  void fullscreen_window() noexcept;
  void restore_fullscreen_window() noexcept;

  auto is_use_title_bar_now() const noexcept { return _wnd->cfg().display_title_bar && !_wnd->is_fullscreen(); }
  void draw_on_title_bar(std::move_only_function<void(float2)>&& func) noexcept;

  auto get_window_context(HWND handle) noexcept { assert(_wnd_names.contains(handle)); return &_wnd_ctxs[_wnd_names[handle]]; }
  void clear_fullscreen_window() noexcept { _fullscreen_window_need_clear = true; }
  void close_window(HWND handle) noexcept { get_window_context(handle)->is_closed = true; }
  void clear_state() noexcept;
  void clear_state_unfocused_window() noexcept;

  auto window() const noexcept { return _wnd; }
  auto frame_data() noexcept { return &_wnd_ctx->frame_data; }

  void postprocess() const noexcept;

  void scissor_area_begin(Rect rect) noexcept;
  void scissor_area_end() noexcept;
  void viewport_begin(StringID name, Rect rect) noexcept;
  void viewport_end();

private:
  void update_window_config(WindowConfig const& cfg) noexcept;

  void preprocess_render() noexcept;
  void postprocess_render() noexcept;

  void add_title_bar() noexcept;
  void fullscreen_process() noexcept;
  void window_shadow_wireframe_process(WindowContext& wnd_ctx, renderer::Window const& wnd, Rect scissor_rect) noexcept;

private:
  //
  // misc
  //
  std::unordered_map<StringID, WindowContext> _wnd_ctxs;
  std::unordered_map<HWND, StringID>          _wnd_names;

  renderer::Window* _wnd{};
  WindowContext*    _wnd_ctx{};
  HWND              _fullscreen_window{};
  bool              _fullscreen_window_need_clear{};

  bool   _call_begin{};
  double _delta_time{};

  std::move_only_function<void(float2)> _user_title_bar_draw_func;

public:
  //
  // widget ids
  //
  std::unordered_set<size_t>        _hovered_widget_ids;
  size_t                            _last_hovered_widget_id{};
  size_t                            _prev_hovered_widget_id{};
  std::unordered_map<size_t, Tween> _tweens;
  std::unordered_set<size_t>        _ids;

  //
  // mouse state
  //
public:
  HWND                cursor_on_window{};
  HWND                mouse_down_window{};
  HWND                mouse_up_window{};
  std::optional<int2> mouse_down_window_pos;
  std::optional<int2> mouse_up_window_pos;
  bool                is_move_from_maximize{};
  bool                draw_title_bar{};
  bool                is_last_mouse_up{};

  //
  // button state
  //
private:
  struct ButtonState
  {
    size_t id{};
    float2 left_top{};
    float2 right_bottom{};
    bool   move_out{};
  } _btn_state;
public:
  void add_mouse_left_button_state(size_t id, float2 left_top, float2 right_bottom) noexcept;
  auto is_cursor_move_out(size_t id) noexcept -> bool;
  auto is_last_mouse_down_id(size_t id) const noexcept { return _btn_state.id == id; }

  //
  // Shape Operation
  //
private:
  std::vector<Rect> _scissor_area_stack;

  struct ViewPortInfo
  {
    float2 offset;
    float  scale{ 1 };
    uint call_cnt{};
  };

  struct ViewportScope
  {
    Rect   rect;
    Rect   screen_rect;
    Rect   interaction_rect;
    Matrix transform;
    bool   using_transform{};
  };

  std::unordered_map<uint64, ViewPortInfo> _viewport_infos;
  std::vector<ViewportScope>               _viewport_stack;
  size_t                                   _dragging_viewport_id{};
)

}
