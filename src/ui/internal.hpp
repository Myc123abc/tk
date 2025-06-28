#pragma once

#include "tk/GraphicsEngine/GraphicsEngine.hpp"

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

  std::vector<glm::vec2>                  points;
  std::vector<graphics_engine::ShapeInfo> shape_infos;

  std::vector<Layout> layouts;
  Layout* last_layout{};

  bool     path_begining{};
  uint32_t path_idx{};

  glm::vec2 mouse_pos{};
  glm::vec2 drag_start_pos{};
  glm::vec2 drag_end_pos{};
  bool      click_finish{};

  std::pair<std::string, std::string> current_hovered_widget{};
  std::vector<glm::vec2>              current_hovered_widget_rect{};
  std::pair<std::string, std::string> last_hovered_widget{};

  // TODO: tmp
  glm::vec4 a, p;
};

inline auto get_ctx()
{
  static auto ctx = new ui_context();
  return ctx;
}

void render();
void clear();
void text_mask_render();

void event_process();

}}