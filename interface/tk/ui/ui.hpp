//
// ui
// 
// use imgui mode
// for mouse handle, I only use a bound rectangle to detection for simply
//
// draw way use sdf
//

#pragma once

#include "../config.hpp"
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
TK_API void begin(std::string_view name, glm::vec2 const& pos = {});
TK_API inline void begin(std::string_view name, float x, float y) { begin(name, { x, y }); }

/**
* end a layout
*/
TK_API void end();

////////////////////////////////////////////////////////////////////////////////
//                                Shape
////////////////////////////////////////////////////////////////////////////////

/**
 * draw line
 * @param p0 start point
 * @param p1 end point
 * @param color
 */
TK_API void line(glm::vec2 const& p0, glm::vec2 const& p1, uint32_t color = 0);

/**
 * draw rectangle
 * @param left_top left upper corner
 * @param right_bottom right down corner
 * @param color rgba
 * @param thickness
 */
TK_API inline void rectangle(glm::vec2 const& left_top, glm::vec2 const& right_bottom, uint32_t color = 0, uint32_t thickness = 0);

/**
 * draw triangle
 * @param p0
 * @param p1
 * @param p2
 * @param color rgba
 * @param thickness
 */
TK_API inline void triangle(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, uint32_t color = 0, uint32_t thickness = 0);

/**
 * draw polygon
 * @param points
 * @param color rgba
 * @param thickness
 */
TK_API void polygon(std::vector<glm::vec2> const& points, uint32_t color = 0, uint32_t thickness = 0);

/**
 * draw circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
TK_API void circle(glm::vec2 const& center, float radius, uint32_t color = 0, uint32_t thickness = 0);

/**
 * draw quadratic bezier
 * @param p0
 * @param p1
 * @param p2
 * @param color rgba
 */
TK_API void bezier(glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2, uint32_t color = 0);

/*
 * start path shape, can only use line and bezier now.
 * and set_operation can only use with path_begin
 */
TK_API void path_begin();
TK_API void path_end(uint32_t color = 0, uint32_t thickness = 0);

// get union of shapes
TK_API void union_begin();
TK_API void union_end(uint32_t color, uint32_t thickness = 0);

/**
 * draw text
 * @param text
 * @param pos left top of text
 * @param size
 * @param inner_color
 * @param italic
 * @param bold
 * @param outer_color alpha not 0 then draw outline
 * @return extent of text
 */
TK_API auto text(std::string_view text, glm::vec2 const& pos, float size, uint32_t inner_color, bool italic = false, bool bold = false, uint32_t outer_color = 0) -> std::pair<glm::vec2, glm::vec2>;

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
TK_API bool button(std::string_view name, type::shape shape, std::vector<glm::vec2> const& data, uint32_t color, uint32_t thickness = 0);

/**
 * create a clickable rectangle area
 * @param name name for id in layout (different layout can have same name widget)
 * @param pos0 position on left upper
 * @param pos1 position on right lower
 * @return true if area is clicked
 */
TK_API bool click_area(std::string_view name, glm::vec2 const& pos0, glm::vec2 const& pos1);

/**
 * judge whether hover on specific widget by name of current layer
 * @param name name of widget of current layer
 * @return true if hovering
 */
TK_API bool is_hover_on(std::string_view name);

}}