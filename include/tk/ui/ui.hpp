//
// UI library
//
// implement by graphics engine
//
// HACK:
// 1. layout managed by ui, but button managed by user
//

#pragma once

#include "../GraphicsEngine/GraphicsEngine.hpp"
#include "UIWidget.hpp"

namespace tk { namespace ui {

  void init(graphics_engine::GraphicsEngine* engine);
  auto create_layout() -> Layout*;
  auto create_button(uint32_t width, uint32_t height, Color color) -> Button*;

  void put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y);
  void put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y);

  // HACK: use _layouts update matrix every frame, maybe performance suck 
  void render();

} }
