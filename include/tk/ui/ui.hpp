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
   * @param color color of rectangle, which use RGBA
   */
  void rectangle(glm::vec2 const& pos, glm::vec2 const& extent, uint32_t color);

  /**
   * draw a triangle
   * three points should be clockwise
   * @param p1 point 1
   * @param p2 point 2
   * @param p3 point 3
   * @param color color of triangle
   * @param thickness if thickness > 0, will enable stroke draw
   */
  void triangle(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, uint32_t color, float thickness = 0.f);

  /**
   * path draw, add point
   * please promise points are clockwise to connect a shape
   * @param point point of path
   */
  void path_line_to(glm::vec2 point);

  /**
   * draw stroke using added points
   * @param color color of stroke
   * @param thickness thickness of stroke
   * @param is_closed whether is closed shape
   */
  void path_stroke(uint32_t color, float thickness, bool is_closed);
}}