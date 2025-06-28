//
// TODO:
// 1. implement shape cover mouse hit per layer
// 2. implement layer cover mouse hit
//

#pragma once

#include "tk/GraphicsEngine/GraphicsEngine.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <unordered_map>

namespace tk { namespace ui {

struct Widget
{
  std::string name;
};

struct Layout
{
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

  std::unordered_map<std::string, Layout> layouts;
  Layout* last_layout{};

  bool     path_begining{};
  uint32_t path_idx{};

  glm::vec2 mouse_pos{};
  glm::vec2 drag_start_pos{};
  glm::vec2 drag_end_pos{};
  bool      click_finish{};

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