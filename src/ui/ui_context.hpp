#pragma once

#include "frame_data.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/config.hpp"
#include "../util/message_queue.hpp"
#include "config.hpp"
#include "ui/ui.hpp"
#include "ui/lerpolator.hpp"
#include "../util/singleton.hpp"
#include "../util/double_buffer.hpp"

#include <windows.h>

#include <array>
#include <unordered_map>
#include <string>
#include <optional>
#include <unordered_set>

namespace tk::ui {

auto is_hover_on(glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;
auto is_caps_locked() noexcept -> bool;

struct Window
{
  renderer::WindowSnapshot snap;

  auto extent() const noexcept { return glm::vec<2, uint32_t>{ snap.width + shadow_thickness() * 2, snap.height + shadow_thickness() * 2 }; }

  auto rect() const noexcept { return RECT{ snap.x, snap.y, static_cast<LONG>(snap.x + snap.width), static_cast<LONG>(snap.y + snap.height) }; }

  auto cursor_pos() const noexcept -> glm::vec<2, int>
  {
    auto pos = renderer::get_cursor_pos();
    return { pos.x - snap.x, pos.y - snap.y };
  }

  auto real_cursor_pos() const noexcept -> glm::vec<2, int>
  {
    return cursor_pos() + glm::vec<2, int>{ renderer::Window_Shadow_Thickness };
  }

  auto shadow_thickness() const noexcept -> LONG { return renderer::Window_Shadow_Thickness * snap.scale; }
  auto real_pos() const noexcept -> glm::vec2 { return { snap.x - shadow_thickness(), snap.y - shadow_thickness() }; }
  auto pos()      const noexcept -> glm::vec2 { return { snap.x, snap.y }; }
  auto real_rect()   const noexcept { return RECT{ snap.x                                  - shadow_thickness(),
                                                   snap.y                                  - shadow_thickness(),
                                                   static_cast<LONG>(snap.x + snap.width)  + shadow_thickness(),
                                                   static_cast<LONG>(snap.y + snap.height) + shadow_thickness() }; }

  auto is_moving_or_resizing() const noexcept { return snap.moving || snap.resizing; }
  auto is_active() const noexcept { return GetForegroundWindow() == snap.handle; }

  // ui window content
  bool         is_called{};
  bool         can_be_closed{};
  bool         is_closed{};
  WindowConfig cfg{};
  glm::vec2    render_pos{};
  bool         need_clear{};

  uint32_t                                     frame_index{};
  std::array<FrameData, renderer::Frame_Count> frame_datas;

  auto frame_data() noexcept { return &frame_datas[frame_index];                     }
  void next_frame() noexcept { frame_index = (frame_index + 1) % frame_datas.size(); }

  DoubleBuffer<std::vector<RECT>> move_invalid_areas;
  void add_move_invald_areas(RECT rect) noexcept { move_invalid_areas.data().emplace_back(rect); }

  DoubleBuffer<WindowConfig> cfgs;

  bool set_fullscreen{};
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

  auto frame_data() noexcept { return _window->frame_data(); }

  void render() noexcept;

  void begin_path() noexcept;
  void end_path(Color color, float thickness) noexcept;
  void begin_union() noexcept;
  void end_union(Color color, float thickness) noexcept;

  auto is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

  void set_render_pos(int x, int y) noexcept { _window->render_pos = { x + renderer::Window_Shadow_Thickness, y + renderer::Window_Shadow_Thickness }; }
  auto get_render_pos() const noexcept { return _window->render_pos; }
  auto get_scale() const noexcept { return _window->snap.scale; }

  void render_on(int x, int y, std::move_only_function<void()>&& func) noexcept;

  auto generic_id(std::string_view name) noexcept -> size_t;
  auto get_id(std::string_view name) const noexcept -> size_t;

  auto delta_time() const noexcept { return _delta_time; }

  auto get_lerpolator(size_t id, double duration) noexcept -> Lerpolator*;
  void remove_lerpolator(size_t id) noexcept;
  void reset_lerpolator(size_t id) noexcept;
  auto lerp_ping_pong(bool b, size_t id, double duration) noexcept -> double;

private:
  auto& get_window(HWND handle) noexcept { return _windows[_window_names[handle]]; }
public:
  auto& access_move_invalid_areas(HWND handle) noexcept { return get_window(handle).move_invalid_areas.access(); }
  auto const& access_window_cfg(HWND handle) noexcept { return get_window(handle).cfgs.access(); }

  void image(std::string_view path, glm::vec2 left_top, glm::vec2 right_bottom, uint8_t alpha) noexcept;
  auto text(std::string_view text, glm::vec2 pos, float size, Color inner_color, FontStyle style, Color outer_color) noexcept -> glm::vec2;

  void fullscreen_window() noexcept;
  void restore_fullscreen_window() noexcept;
  auto is_use_title_bar_now() const noexcept { return _window->cfg.display_title_bar && !_window->snap.fullscreen_window; }

private:
  void update_window_config(WindowConfig const& cfg) noexcept;

  void preprocess_render() noexcept;
  void postprocess_render() noexcept;

  void add_title_bar() noexcept;
  void fullscreen_process() noexcept;
  void window_shadow_wireframe_process(Window& wnd, RECT scissor_rect) noexcept;

public:
  Window*                                 _window{};
private:
  Window                                  _fullscreen_window{};
  std::unordered_map<std::string, Window> _windows;
  std::unordered_map<HWND, std::string>   _window_names;
  bool                                    _call_begin{};
  bool                                    _path_begin{};
  bool                                    _union_begin{};
  double                                  _delta_time{};

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

////////////////////////////////////////////////////////////////////////////////
///                              Message Process
////////////////////////////////////////////////////////////////////////////////

public:
  struct Message_Window_Close
  {
    HWND handle{};
  };
  
  struct Message_Cursor_On_Window
  {
    HWND handle{};
  };

  struct Message_Window_Update
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Update_Moving
  {
    HWND handle{};
    bool moving{};
    int  x{};
    int  y{};
  };

  struct Message_Update_Resizing
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Resize_End
  {
    HWND handle{};
  };

  struct Message_Window_Maximize
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Restore
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Moving_From_Maximize
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Moving_From_Maximize_End
  {
    HWND handle{};
    int  x{};
    int  y{};
  };

  struct Message_Interruption {};
  struct Message_Update_Fullscreen_Window
  {
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Scale_Change
  {
    HWND     handle{};
    float    scale{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Fullscreen
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Restore_Fullscreen
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  struct Message_Window_Cancel_Fullscreen_Maximize
  {
    HWND     handle{};
    int      x{};
    int      y{};
    uint32_t width{};
    uint32_t height{};
  };

  using Message = std::variant<
    Message_Window_Close,
    Message_Cursor_On_Window,
    Message_Window_Update,
    Message_Update_Moving,
    Message_Update_Resizing,
    Message_Resize_End,
    Message_Window_Maximize,
    Message_Window_Restore,
    Message_Window_Moving_From_Maximize,
    Message_Window_Moving_From_Maximize_End,
    Message_Interruption,
    Message_Update_Fullscreen_Window,
    Message_Scale_Change,
    Message_Window_Fullscreen,
    Message_Window_Restore_Fullscreen,
    Message_Window_Cancel_Fullscreen_Maximize
  >;

  void send_message(Message&& msg) noexcept
  {
    _msg_queue.send(std::move(msg));
  }

private:
  struct MessageHandler
  {
    UIContext& ctx;
    void operator()(Message_Window_Close const& msg) const noexcept;
    void operator()(Message_Cursor_On_Window const& msg) const noexcept;
    void operator()(Message_Window_Update const& msg) const noexcept;
    void operator()(Message_Update_Moving const& msg) const noexcept;
    void operator()(Message_Update_Resizing const& msg) const noexcept;
    void operator()(Message_Resize_End const& msg) const noexcept;
    void operator()(Message_Window_Maximize const& msg) const noexcept;
    void operator()(Message_Window_Restore const& msg) const noexcept;
    void operator()(Message_Window_Moving_From_Maximize const& msg) const noexcept;
    void operator()(Message_Window_Moving_From_Maximize_End const& msg) const noexcept;
    void operator()(Message_Interruption const& msg) const noexcept;
    void operator()(Message_Update_Fullscreen_Window const& msg) const noexcept;
    void operator()(Message_Scale_Change const& msg) const noexcept;
    void operator()(Message_Window_Fullscreen const& msg) const noexcept;
    void operator()(Message_Window_Restore_Fullscreen const& msg) const noexcept;
    void operator()(Message_Window_Cancel_Fullscreen_Maximize const& msg) const noexcept;
  };
  MessageQueue<Message, UI_Message_Queue_Capacity> _msg_queue;
)

}
