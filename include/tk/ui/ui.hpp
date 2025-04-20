//
// UI library
//
// implement by graphics engine
//
// TODO:
// 1. no delete, so created ui will be storage forever.
// 2. how to handle ui in same place and same depth.
// 3. use tree manage ui
// 4. use quadtree handle widgets search
// 5. test circle for extend shape type
// 6. seperate click with widget
//
// INFO:
// 1. Default ui have a layout with single quard background use OneDark.
//    It's depth value is 0.f. And other ui widgets you create default 
//    depth start with 0.1f.
//
// HACK:
// 1. Don't set depth value equal 0.f, it will conflict with background,
//    we no handle same place same depth value case.
//

#pragma once

#include "../GraphicsEngine/GraphicsEngine.hpp"
#include "UIWidget.hpp"

namespace tk { namespace ui {

  void init(graphics_engine::GraphicsEngine* engine);
  auto create_layout() -> Layout*;
  auto create_button(ShapeType shape, Color color, std::initializer_list<uint32_t> values) -> Button*;

  void put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y);
  void put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y);

  // HACK: use _layouts update matrix every frame, maybe performance suck 
  void render();

  void remove(UIWidget* widget, Layout* layout);
} }
