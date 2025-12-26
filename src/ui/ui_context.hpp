#pragma once

#include "../renderer/resource/render_data.hpp"
#include "../renderer/config.hpp"
#include "../util/message_queue.hpp"
#include "config.hpp"

#include <windows.h>

#include <array>
#include <unordered_map>
#include <string>

namespace tk { namespace ui {

struct Window
{
  HWND handle; 
  bool is_called{};
  bool can_be_closed{};
  bool is_closed{};
  bool no_frame_can_use{};

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

  void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed) noexcept;
  void end() noexcept;

  void check_draw() const noexcept;

  void render() noexcept;

  auto render_data() noexcept { return _window->data(); }

  void add_vertices_indices(std::pair<glm::vec2, glm::vec2> const& bounding_rectangle) noexcept;
  void add_shape_property(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept;
  void add_shape(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> const& bounding_rectangle) noexcept;

  auto is_no_frame_can_use() const noexcept { return _window->no_frame_can_use; }

private:
  bool                                    _call_begin{};
  std::unordered_map<std::string, Window> _windows;
  Window*                                 _window{};
  uint32_t                                _shape_properties_offset{};
  uint16_t                                _idx_beg{};

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
    void operator()(Message_Window_Close const& msg) const noexcept;
  };
  MessageQueue<Message, UI_Message_Queue_Capacity> _msg_queue;
};

inline static auto& g_ui_ctx{ UIContext::instance() };

}}
