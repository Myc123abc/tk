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

void line(glm::vec2 start, glm::vec2 end, uint32_t color, float thickness = 0.f);

/**
 * draw polygon
 * @param points points of polygon. If it's filled polygon, need to be counter clockwise.
 * @param color color of oriented half plane
 * @param thickness if thickness > 0, will enable stroke draw
 */
void polygon(std::vector<glm::vec2> const& points, uint32_t color, float thickness = 0.f);

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