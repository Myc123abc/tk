#pragma once

#include "ui/ui.hpp"
#include "../util/align.hpp"
#include "image_manager.hpp"

#include <optional>
#include <semaphore>

namespace tk { namespace ui {

enum class CommandType : uint16_t
{
  draw_rectangle,
  draw_triangle,
  draw_circle,
  draw_line,
  draw_bezier,

  add_discard_rectangle,

  begin_path,
  end_path,
  begin_union,
  end_union,

  enable_tmp_color,
  disable_tmp_color,

  image,

  set_scissor_rect,
  set_window_pos
};

struct CommandHeader
{
  CommandType type{};
  uint16_t    offset{};
  uint32_t    size{};
};

struct RectangleInfo
{
  glm::vec2 left_top{};
  glm::vec2 right_bottom{};
  Color     color{};
  float     thickness{};
};

struct TriangleInfo
{
  glm::vec2 p0{};
  glm::vec2 p1{};
  glm::vec2 p2{};
  Color     color{};
  float     thickness{};
};

struct CircleInfo
{
  glm::vec2 center{};
  float     radius{};
  Color     color{};
  float     thickness{};
};

struct LineInfo
{
  glm::vec2 p0{};
  glm::vec2 p1{};
  Color     color{};
};

struct BezierInfo
{
  glm::vec2 p0{};
  glm::vec2 p1{};
  glm::vec2 p2{};
  Color     color{};
};

struct DiscardRectangleInfo
{
  glm::vec2 left_top{};
  glm::vec2 right_bottom{};
};

struct EndPathInfo
{
  Color color{};
  float thickness{};
};

struct EndUnionInfo
{
  Color color{};
  float thickness{};
};

struct TmpColorInfo
{
  glm::vec4 color{};
};

struct ImageInfo
{
  ImageHandle handle{};
  glm::vec2   left_top{};
  glm::vec2   right_bottom{};
  uint8_t     alpha{}; 
};

struct ScissorRectInfo
{
  RECT rect{};
};

struct WindowPosInfo
{
  glm::vec2 pos{};
};

class CommandList
{
public:
  CommandList()                              = default;
  ~CommandList()                             = default;
  CommandList(CommandList const&)            = delete;
  CommandList(CommandList&&)                 = delete;
  CommandList& operator=(CommandList const&) = delete;
  CommandList& operator=(CommandList&&)      = delete;

private:
  enum class State
  {
    record_complete,
    recording,
  };

  void push(CommandType type) noexcept
  {
    assert(_state == State::recording);

    auto offset_header = align(_buf.size(), alignof(CommandHeader));
    auto offset_end    = align(offset_header + sizeof(CommandHeader), alignof(CommandHeader));

    _buf.resize(offset_end);

    auto header = CommandHeader{ type, 0, static_cast<uint32_t>(offset_end - offset_header) };
    memcpy(_buf.data() + offset_header, &header, sizeof(header));
  }

  template <typename T>
  requires std::is_trivially_copyable_v<T>
  void push(CommandType type, T&& obj) noexcept
  {
    assert(_state == State::recording);

    auto offset_header = align(_buf.size(), alignof(CommandHeader));
    auto offset_data   = align(offset_header + sizeof(CommandHeader), alignof(T));
    auto offset_end    = align(offset_data + sizeof(T), alignof(CommandHeader));

    _buf.resize(offset_end);

    auto header = CommandHeader{ type, static_cast<uint16_t>(offset_data - offset_header), static_cast<uint32_t>(offset_end - offset_header) };
    memcpy(_buf.data() + offset_header, &header, sizeof(header));
    memcpy(_buf.data() + offset_data, &obj, sizeof(T));
  }

public:
  void reset() noexcept
  {
    assert(_state == State::record_complete);
    _state  = State::recording;
    _offset = {};
    _buf.clear();
  }

  auto wait() noexcept -> CommandList&
  {
    _sem.acquire();
    return *this;
  }

  void notify() noexcept { _sem.release(); }

  void draw_rectangle(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
  {
    push(CommandType::draw_rectangle, RectangleInfo{ left_top, right_bottom, color, thickness });
  }

  void draw_triangle(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color, float thickness) noexcept
  {
    push(CommandType::draw_triangle, TriangleInfo{ p0, p1, p2, color, thickness });
  }

  void draw_circle(glm::vec2 center, float radius, Color color, float thickness) noexcept
  {
    push(CommandType::draw_circle, CircleInfo{ center, radius, color, thickness });
  }

  void draw_line(glm::vec2 p0, glm::vec2 p1, Color color) noexcept
  {
    push(CommandType::draw_line, LineInfo{ p0, p1, color });
  }

  void draw_bezier(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, Color color) noexcept
  {
    push(CommandType::draw_bezier, BezierInfo{ p0, p1, p2, color });
  }

  void add_discard_rectangle(glm::vec2 left_top, glm::vec2 right_bottom) noexcept
  {
    push(CommandType::add_discard_rectangle, DiscardRectangleInfo{ left_top, right_bottom });
  }

  void begin_path() noexcept
  {
    push(CommandType::begin_path);
  }

  void end_path(Color color, float thickness) noexcept
  {
    push(CommandType::end_path, EndPathInfo{ color, thickness });
  }

  void begin_union() noexcept
  {
    push(CommandType::begin_union);
  }

  void end_union(Color color, float thickness) noexcept
  {
    push(CommandType::end_union, EndUnionInfo{ color, thickness });
  }

  void enable_tmp_color(glm::vec4 color) noexcept
  {
    push(CommandType::enable_tmp_color, TmpColorInfo{ color });
  }

  void disable_tmp_color() noexcept
  {
    push(CommandType::disable_tmp_color);
  }

  void image(ImageHandle handle, glm::vec2 left_top, glm::vec2 right_bottom, uint8_t alpha) noexcept
  {
    push(CommandType::image, ImageInfo{ handle, left_top, right_bottom, alpha });
  } 

  void set_scissor_rect(RECT rect) noexcept
  {
    push(CommandType::set_scissor_rect, ScissorRectInfo{ rect });
  }

  void set_window_pos(glm::vec2 pos) noexcept
  {
    push(CommandType::set_window_pos, WindowPosInfo{ pos });
  }

  auto submit() noexcept -> CommandList&
  {
    assert(_state == State::recording);
    _state = State::record_complete;
    return *this;
  }

  auto empty() const noexcept { return _buf.empty(); }

  auto pop() noexcept -> std::optional<std::pair<CommandType, std::byte*>>
  {
    assert(_state == State::record_complete && !empty());
    if (_offset >= _buf.size()) return {};
    auto header = reinterpret_cast<CommandHeader*>(_buf.data() + _offset);
    _offset += header->size;
    return std::make_pair(header->type, reinterpret_cast<std::byte*>(header) + header->offset);
  }

private:
  std::vector<std::byte> _buf;
  State                  _state{};
  uint32_t               _offset{};
  std::binary_semaphore  _sem{ 1 };
};

}}
