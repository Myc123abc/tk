//
// ui
// 
// use imgui mode
//

#pragma once

#include <glm/glm.hpp>

namespace tk { namespace ui {

  /**
   * start a new layout
   * @param pos position of the window (TODO: currently, we only have a signle main window)
   */
  void begin(glm::vec2 const& pos);
  inline void begin(float x, float y) { begin({ x, y }); }

  /**
   * end a layout
   */
  void end();

  /**
   * draw a rectangle
   * @param pos position of rectangle in a layout (which is a pair of begin and end)
   * @param extent the width and height of this rectangle
   * @param color color of rectangle, which use ARGB
   */
void rectangle(glm::vec2 const& pos, glm::vec2 const& extent, uint32_t color);

}}