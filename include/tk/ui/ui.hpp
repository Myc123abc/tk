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
#include <string_view>

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
* draw a rectangle
* @param pos0 position on left upper
* @param pos1 position on right lower
* @param color color of rectangle, which use RGBA
* @param thickness if thickness > 0, will enable stroke draw
*/
void rectangle(glm::vec2 const& pos0, glm::vec2 const& pos1, uint32_t color, float thickness = 0.f);

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
inline void path_line_to(float x, float y) { path_line_to({x, y}); };

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
 * @param name name for id in layout (different layout can have same name widget)
 * @param shape shape of button
 * @param data data of shape, number of data should be right, such as triangle have three data
 * @param color color of button
 * @param thickness thickness of button's shape
 * @return true if button is clicked
 */
bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, float thickness = 0.f);

/**
 * create a clickable rectangle area
 * @param name name for id in layout (different layout can have same name widget)
 * @param pos0 position on left upper
 * @param pos1 position on right lower
 */
bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1);

}}