export module tk.ui:geometry;

import :common;
import tk.ui.color;
import tk.base;
import tk.flag;

export namespace tk::ui {

/**
 * draw a rectangle
 * @param left_top left upper corner
 * @param right_bottom right down corner
 * @param color
 * @param thickness
 * @param rounding corner radius
 * @param flags rounded corner selection
 */
void rectangle(float2 left_top, float2 right_bottom, Color color = {}, float thickness = {}, float rounding = {}, Flag<CornerFlag> flags = CornerFlag::all) noexcept;

/**
 * draw a triangle (clockwise)
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void triangle(float2 p0, float2 p1, float2 p2, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a circle
 * @param center
 * @param radius
 * @param color
 * @param thickness
 */
void circle(float2 center, float radius, Color color = {}, float thickness = {}) noexcept;

/**
 * draw a line
 * @param p0
 * @param p1
 * @param color
 * @param thickness
 */
void line(float2 p0, float2 p1, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw an arc
 * @param center
 * @param p0
 * @param p1
 * @param ccw whether counter clockwise
 * @param color
 * @param thickness
 */
void arc(float2 center, float2 p0, float2 p1, bool ccw, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw a quad bezier
 * @param p0
 * @param p1
 * @param p2
 * @param color
 * @param thickness
 */
void quad_bezier(float2 p0, float2 p1, float2 p2, Color color = {}, float thickness = 1.f) noexcept;

/**
 * draw a cubic bezier
 * @param p0
 * @param p1
 * @param p2
 * @param p3
 * @param color
 * @param thickness
 */
void cubic_bezier(float2 p0, float2 p1, float2 p2, float2 p3, Color color = {}, float thickness = 1.f) noexcept;

}
