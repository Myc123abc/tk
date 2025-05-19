//
// ui
// 
// use imgui mode
// for mouse handle, I only use a bound rectangle to detection for simply
//

#pragma once

#include "tk/type.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
//                                  Misc
////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////
//                                UI
////////////////////////////////////////////////////////////////////////////////

/**
 * button, can be clicked
 * @param shape shape of button
 * @param data data of shape, number of data should be right, such as triangle have three data
 * @param color color of button
 * @param thickness thickness of button's shape
 * @return true if button is clicked
 */
bool button(type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, float thickness = 0.f);

}}