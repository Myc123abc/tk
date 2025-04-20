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
// 5. for circle, how many segments should be set
//
  // TODO: use it in feature and it can be dynamic rendering, default multisample option
//
// INFO:
// 1. Default ui have a layout with single quard background use OneDark.
//    It's depth value is 0.f. And other ui widgets you create default 
//    depth start with 0.1f.
//
// HACK:
// 1. Don't set depth value equal 0.f, it will conflict with background,
//    we no handle same place same depth value case.
// 2. for materials, static load all shapes colors combination maybe too big,
//    or, how to dynamic load materials
//
// INFO: How to dynamic mesh
// When dynamic mesh, in rendering main thread, record create new mesh buffer commands.
// Use vulkan synchornous mechanism to promise no conflict.
// Create new mesh buffer, size if old size plus new objects' size.
// After create finished, replace old buffer and detele old buffer.
//

#pragma once

#include "../GraphicsEngine/GraphicsEngine.hpp"
#include "UIWidget.hpp"

namespace tk { namespace ui {

  void init(graphics_engine::GraphicsEngine* engine);
  auto create_layout() -> Layout*;
  auto create_button(ShapeType shape, glm::vec3 const& color, std::initializer_list<uint32_t> values) -> Button*;

  void put(Layout* layout, tk::Window* window, uint32_t x, uint32_t y);
  void put(UIWidget* widget, Layout* layout, uint32_t x, uint32_t y);

  // HACK: use _layouts update matrix every frame, maybe performance suck 
  void render();

  void remove(UIWidget* widget, Layout* layout);

} }
