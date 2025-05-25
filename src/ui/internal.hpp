#pragma once

#include "tk/GraphicsEngine/GraphicsEngine.hpp"

#include <glm/glm.hpp>

#include <queue>
#include <vector>
#include <string>
#include <map>

namespace tk { namespace ui {

struct Widget
{
  std::string name;
  uint32_t    id          = 0;
  bool        first_click = false;
};

struct Layout
{
  std::string_view name;
  glm::vec2        pos;
  std::vector<graphics_engine::ShapeInfo> shape_infos;
};

struct ui_context
{
  bool               begining = false;
  std::queue<Layout> layouts;
  // TODO: currently, only single main window is being use
  glm::vec2          window_extent;

  graphics_engine::GraphicsEngine* engine = nullptr;

  uint32_t event_type{};

  std::unordered_map<std::string, std::vector<Widget>>      states;
  std::unordered_map<std::string, std::vector<std::string>> call_stack;
};

inline auto& get_ctx()
{
  static ui_context ctx;
  return ctx;
}

void render();

auto generate_id() -> uint32_t;

}}