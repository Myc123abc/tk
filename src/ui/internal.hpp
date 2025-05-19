#pragma once

#include "tk/GraphicsEngine/GraphicsEngine.hpp"

#include <glm/glm.hpp>

#include <queue>
#include <vector>

namespace tk { namespace ui {

struct Layout
{
  glm::vec2 pos;
  std::vector<graphics_engine::GraphicsEngine::IndexInfo> index_infos;
};

struct ui_context
{
  bool               begining = false;
  std::queue<Layout> layouts;
  // TODO: currently, only single main window is being use
  glm::vec2          window_extent;

  graphics_engine::GraphicsEngine*     engine = nullptr;
  std::vector<graphics_engine::Vertex> vertices;
  std::vector<uint16_t>                indices;

  // use for path draw
  std::vector<glm::vec2> points;

  // 0: nothing (up)
  // 1: first down
  // 2: pressed
  int mouse_down_state = 0;

  uint32_t event_type{};
};

inline auto& get_ctx()
{
  static ui_context ctx;
  return ctx;
}

void render();

}}