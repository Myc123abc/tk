//
// ui
// 
// use imgui mode
// for mouse handle, I only use a bound rectangle to detection for simply
//
// draw way use sdf
// primitive:
// 1. line segment
// 2. circle
// 3. oriented half plane (need points as counter clockwise)
//

#pragma once

#include "tk/type.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <string_view>
#include <span>

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
//                                  Misc
////////////////////////////////////////////////////////////////////////////////

/**
* start a new layout
* @param name name of layout, must unique (TODO: in multi-window, can same)
* @param pos position of the window (TODO: currently, we only have a signle main window)
*/
void begin(std::string_view name, glm::vec2 const& pos);
inline void begin(std::string_view name, float x, float y) { begin(name, { x, y }); }

/**
* end a layout
*/
void end();

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

/**
 * draw shape
 * @param type type of shape
 * @param points points of shape
 * @param color color of shape (RGBA)
 * @param thickness thickness of shape (line thickness always 1 pixel)
 */
void shape(type::shape type, std::vector<glm::vec2> const& points, uint32_t color, uint32_t thickness = 0);

/**
 * draw line
 * @param p0 start point
 * @param p1 end point
 * @param color
 */
inline void line(glm::vec2 p0, glm::vec2 p1, uint32_t color)
{ 
  shape(type::shape::line, { p0, p1 }, color);
}

/**
 * draw rectangle
 * @param left_upper left upper corner
 * @param right_down right down corner
 * @param color rgba
 * @param thickness
 */
inline void rectangle(glm::vec2 const& left_upper, glm::vec2 const& right_down, uint32_t color, uint32_t thickness = 0)
{ 
  shape(type::shape::rectangle, { left_upper, right_down }, color, thickness);
}

/**
 * draw triangle
 * @param p0
 * @param p1
 * @param p2
 * @param color rgba
 * @param thickness
 */
inline void triangle(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, uint32_t color, uint32_t thickness = 0)
{ 
  shape(type::shape::triangle, { p0, p1, p2 }, color, thickness);
}

/**
 * draw polygon
 * @param points
 * @param color rgba
 * @param thickness
 */
inline void polygon(std::vector<glm::vec2> const& points, uint32_t color, uint32_t thickness = 0)
{ 
  shape(type::shape::polygon, points, color, thickness);
}

/**
 * draw circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
void circle(glm::vec2 const& center, float radius, uint32_t color, uint32_t thickness = 0);

////////////////////////////////////////////////////////////////////////////////
//                                UI
////////////////////////////////////////////////////////////////////////////////

/**
 * set current shape mix operation with next shape.
 * the final shape operation must be mix (default).
 * the operator for two shapes only use thickness of first shape.
 * @param op operator for current shape with next shape
 */
void set_operation(type::shape_op op);

/**
 * button, can be clicked
 * @param name name for id in layout (different layout can have same name widget)
 * @param shape shape of button
 * @param data data of shape, number of data should be right, such as triangle have three data
 * @param color color of button
 * @param thickness thickness of button's shape
 * @return true if button is clicked
 */
bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, uint32_t thickness = 0);

/**
 * create a clickable rectangle area
 * @param name name for id in layout (different layout can have same name widget)
 * @param pos0 position on left upper
 * @param pos1 position on right lower
 */
bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1);

}}