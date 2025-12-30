#pragma once

#include "../renderer/resource/render_data.hpp"
#include "../renderer/config.hpp"
#include "../util/message_queue.hpp"
#include "config.hpp"
#include "ui/ui.hpp"
#include "../renderer/window/window.hpp"
#include "lerp_animation.hpp"

#include <windows.h>

#include <array>
#include <unordered_map>
#include <string>
#include <optional>
#include <unordered_set>

namespace tk { namespace ui {

struct Window
{
  HWND         handle; 
  bool         is_called{};
  bool         can_be_closed{};
  bool         is_closed{};
  WindowConfig cfg{};
  glm::vec2    render_pos{};

  uint32_t                                                frame_index{};
  std::array<renderer::RenderData, renderer::Frame_Count> datas;

  auto data()       noexcept { return &datas[frame_index];                     }
  void next_frame() noexcept { frame_index = (frame_index + 1) % datas.size(); }
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

  void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed, WindowConfig cfg) noexcept;
  void end() noexcept;

  void check_draw() const noexcept;
  void check_path_draw() const noexcept;
  void check_path_not_draw() const noexcept;

  void render() noexcept;
  auto render_data() noexcept { return _window->data(); }
  auto render_pos() noexcept { return _window->render_pos; }

  void add_vertices_indices(std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;
  void add_shape_property(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept;
  void add_shape(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> bounding_rectangle) noexcept;

  auto add_lerp_anim(size_t id, uint32_t dur) noexcept -> LerpAnimation*;

  auto is_path_draw() const noexcept { return _path_begin; }

  bool is_hover_on(size_t id, glm::vec2 left_top, glm::vec2 right_bottom, LerpAnimation* lerp_anim) noexcept;

  void enable_tmp_color(glm::vec4 color) noexcept { _tmp_color = color; }
  void disable_tmp_color() noexcept { _tmp_color = {}; }
  void set_render_pos(int x, int y) noexcept { _window->render_pos = { x, y }; }
  auto get_render_pos() const noexcept { return _window->render_pos; }

  auto generic_id(std::string_view name) const noexcept -> size_t;

private:
  void update() noexcept;
  void add_title_bar() noexcept;

public:
  renderer::Window const*                 window;
private:
  std::unordered_map<std::string, Window> _windows;
  Window*                                 _window{};
  uint32_t                                _shape_properties_offset{};
  uint16_t                                _idx_beg{};

  bool                                    _call_begin{};
  bool                                    _path_begin{};
public:
  std::vector<float>                      path_data;
  std::vector<glm::vec2>                  path_points;
private:

  struct OperatorShapeRenderData
  {
    renderer::ShapeProperty::Operator op;
    std::vector<glm::vec2>            points;
    uint32_t                          offset{};
  } _op_data;

  std::optional<glm::vec4> _tmp_color;

  Timer                                     _lerp_anim_timer;
  std::unordered_map<size_t, LerpAnimation> _lerp_anims;
  std::vector<size_t>                       _hovered_widget_ids;
  size_t                                    _prev_hovered_widget_id{};

  std::unordered_set<size_t>                _ids;

  // state
public:
  HWND                            cursor_on_window{};
  HWND                            mouse_down_window{};
  HWND                            mouse_up_window{};
  std::optional<glm::vec<2, int>> mouse_down_pos;
  std::optional<glm::vec<2, int>> mouse_up_pos;

////////////////////////////////////////////////////////////////////////////////
///                              Message Process
////////////////////////////////////////////////////////////////////////////////

public:
  struct Message_Window_Close
  {
    HWND handle{};
  };

  using Message = std::variant<
    Message_Window_Close
  >;

  void send_message(Message&& msg) noexcept
  {
    _msg_queue.send(std::move(msg));
  }

private:
  struct MessageHandler
  {
    UIContext& ctx;
    void operator()(Message_Window_Close msg) const noexcept;
  };
  MessageQueue<Message, UI_Message_Queue_Capacity> _msg_queue;
};

inline static auto& g_ui_ctx{ UIContext::instance() };

}}
