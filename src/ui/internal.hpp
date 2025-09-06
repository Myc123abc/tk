#pragma once

#include "../GraphicsEngine/GraphicsEngine.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace tk { namespace ui {

struct Widget
{
  std::string name;
};

struct Layout
{
  std::string         name;
  glm::vec2           pos;
  std::vector<Widget> widgets;
};

struct ui_context
{
  bool               begining{};
  // TODO: currently, only single main window is being use
  glm::vec2          window_extent;

  graphics_engine::GraphicsEngine* engine{};
  Window*                          window{};

  std::vector<Layout> layouts;
  Layout*             last_layout{};

  bool                   path_begining{};
  uint32_t               path_count{};
  std::vector<glm::vec2> path_points;
  uint32_t               path_color{};
  uint32_t               path_offset{};
  std::vector<float>     paritions{};

  glm::vec2 mouse_pos{};
  glm::vec2 drag_start_pos{};
  glm::vec2 drag_end_pos{};
  bool      click_finish{};
  bool      first_down{};

  std::pair<std::string, std::string> current_hovered_widget{};
  std::vector<glm::vec2>              current_hovered_widget_rect{};
  std::pair<std::string, std::string> last_hovered_widget{};

  std::vector<graphics_engine::Vertex>        vertices;
  std::vector<uint16_t>                       indices;
  uint16_t                                    index{};
  std::vector<graphics_engine::ShapeProperty> shape_properties;
  uint32_t                                    shape_offset{};

  std::vector<glm::vec2> op_points;
  uint32_t               op_offset{};
  bool                   union_start{};

  float outline_width{ .05f };
};

inline auto get_ctx()
{
  static auto ctx = new ui_context();
  return ctx;
}

void render();
void clear();

void event_process();

}}