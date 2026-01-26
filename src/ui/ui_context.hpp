#pragma once

#include "../renderer/resource/render_data.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/config.hpp"
#include "../util/message_queue.hpp"
#include "config.hpp"
#include "ui/ui.hpp"
#include "lerpolator.hpp"

#include <windows.h>

#include <array>
#include <unordered_map>
#include <string>
#include <optional>
#include <unordered_set>
#include <deque>

namespace tk { namespace ui {

struct Window
{
  renderer::WindowSnapshot snap;

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

  auto real_pos() const noexcept -> glm::vec2 { return { snap.x - renderer::Window_Shadow_Thickness, snap.y - renderer::Window_Shadow_Thickness }; }
  auto pos()      const noexcept -> glm::vec2 { return { snap.x, snap.y }; }

  auto is_moving_or_resizing() const noexcept { return snap.moving || snap.resizing; }
  auto is_active() const noexcept { return GetForegroundWindow() == snap.handle; }

  // ui window content
  bool         is_called{};
  bool         can_be_closed{};
  bool         is_closed{};
  WindowConfig cfg{};
  glm::vec2    render_pos{};
  bool         need_clear{};

  uint32_t                                                      frame_index{};
  std::array<renderer::RenderDataHandle, renderer::Frame_Count> datas;

  auto data()       noexcept { return &renderer::g_render_data_pool[datas[frame_index]]; }
  void next_frame() noexcept { frame_index = (frame_index + 1) % datas.size();           }

  std::array<std::vector<RECT>, 2> move_invalid_areas{};
  std::atomic_uint32_t             move_invalid_areas_idx{};

  void add_move_invald_areas(RECT rect) noexcept;
  void clear_move_invalid_areas() noexcept;
  void switch_move_invalid_areas() noexcept;
  auto access_move_invliad_areas() noexcept -> std::vector<RECT>&;
};

class UIContext
{
private:
  UIContext()                           = default;
  ~UIContext()                          = default;
public:
  UIContext(UIContext const&)            = delete;
  UIContext(UIContext&&)                 = delete;
  UIContext& operator=(UIContext const&) = delete;
  UIContext& operator=(UIContext&&)      = delete;

  static auto instance() noexcept -> UIContext&
  {
    static UIContext instance;
    return instance;
  }

  void init() noexcept;
  void destroy() noexcept;

  void close_window() noexcept;

  void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg) noexcept;
  void end() noexcept;

  void check_draw() const noexcept;
  void check_path_draw() const noexcept;
  void check_path_not_draw() const noexcept;

  void render() noexcept;

  void add_vertices_indices(std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;
  void add_shape_property(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept;
  void add_shape(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;

  auto is_path_draw() const noexcept { return _path_begin; }
  auto is_union_draw() const noexcept { return _union_begin; }

  auto is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept -> bool;

  void enable_tmp_color(glm::vec4 color) noexcept { _tmp_color = color; }
  void disable_tmp_color() noexcept { _tmp_color = {}; }
  auto get_render_data() noexcept { return _window->data(); }
  void set_render_pos(int x, int y) noexcept { _window->render_pos = { x + renderer::Window_Shadow_Thickness, y + renderer::Window_Shadow_Thickness }; }
  auto get_render_pos() const noexcept { return _window->render_pos; }
  auto get_mouse_state() const noexcept { return _mouse_state; }

  void render_on(int x, int y, std::move_only_function<void()>&& func) noexcept;

  auto generic_id(std::string_view name) const noexcept -> size_t;

  auto delta_time() const noexcept { return _delta_time; }

  auto get_lerpolator(size_t id, double duration) noexcept -> Lerpolator*;
  void remove_lerpolator(size_t id) noexcept;
  auto ping_pong_lerp(bool b, size_t id, double duration) noexcept -> double;

  auto access_move_invalid_areas(HWND handle) noexcept -> std::vector<RECT>&;

private:
  void update() noexcept;

  auto& get_window(HWND handle) noexcept { return _windows[_window_names[handle]]; }

  void add_title_bar() noexcept;

public:
  Window*                                 _window{};
private:
  Window                                  _fullscreen_window{};
  std::unordered_map<std::string, Window> _windows;
  std::unordered_map<HWND, std::string>   _window_names;
  uint32_t                                _shape_properties_offset{};
  uint16_t                                _idx_beg{};

  bool                                    _call_begin{};
  bool                                    _path_begin{};
  bool                                    _union_begin{};

public:
  std::vector<float>                      path_data;
  std::vector<glm::vec2>                  path_points;
private:

  struct OperatorShapeRenderData
  {
    renderer::ShapeProperty::Operator op;
    std::vector<glm::vec2>            points;
    uint32_t                          offset{};
  };
  OperatorShapeRenderData  _op_data;
  std::optional<glm::vec4> _tmp_color;
  double                   _delta_time{};

  //
  // widget ids
  //
  std::unordered_set<size_t>             _hovered_widget_ids;
  size_t                                 _last_hovered_widget_id{};
  size_t                                 _prev_hovered_widget_id{};
  std::unordered_map<size_t, Lerpolator> _lerpolators;
  std::unordered_set<size_t>             _ids;

  //
  // state
  //
public:
  HWND                            cursor_on_window{};
  HWND                            mouse_down_window{};
  HWND                            mouse_up_window{};
  std::optional<glm::vec<2, int>> mouse_down_pos;
  std::optional<glm::vec<2, int>> mouse_up_pos;
  bool                            is_move_from_maximize{};
  bool                            draw_title_bar{};
private:
  window::MouseState              _mouse_state{};

  // button state
  struct ButtonState
  {
    size_t    id{};
    glm::vec2 left_top{};
    glm::vec2 right_bottom{};
    bool      move_out{};
  } _btn_state;
  bool _interrupte{};
public:
  void add_mouse_state(size_t id, glm::vec2 left_top, glm::vec2 right_bottom) noexcept;
  auto is_cursor_move_out(size_t id) noexcept -> bool;

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

  struct Message_Update_Mouse_State
  {
    window::MouseState state{};
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

  using Message = std::variant<
    Message_Window_Close,
    Message_Cursor_On_Window,
    Message_Update_Mouse_State,
    Message_Update_Moving,
    Message_Update_Resizing,
    Message_Resize_End,
    Message_Window_Maximize,
    Message_Window_Restore,
    Message_Window_Moving_From_Maximize,
    Message_Window_Moving_From_Maximize_End,
    Message_Interruption,
    Message_Update_Fullscreen_Window
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
    void operator()(Message_Update_Mouse_State const& msg) const noexcept;
    void operator()(Message_Update_Moving const& msg) const noexcept;
    void operator()(Message_Update_Resizing const& msg) const noexcept;
    void operator()(Message_Resize_End const& msg) const noexcept;
    void operator()(Message_Window_Maximize const& msg) const noexcept;
    void operator()(Message_Window_Restore const& msg) const noexcept;
    void operator()(Message_Window_Moving_From_Maximize const& msg) const noexcept;
    void operator()(Message_Window_Moving_From_Maximize_End const& msg) const noexcept;
    void operator()(Message_Interruption const& msg) const noexcept;
    void operator()(Message_Update_Fullscreen_Window const& msg) const noexcept;
  };
  MessageQueue<Message, UI_Message_Queue_Capacity> _msg_queue;
  std::deque<Message_Update_Mouse_State>           _mouse_state_queue;

  void process_mouse_state() noexcept;
};

inline static auto& g_ui_ctx{ UIContext::instance() };

}}
