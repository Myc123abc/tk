export module tk.ui:path;

import tk.ui.color;
import tk.base;
import tk.string_id;

export namespace tk::ui {

/**
 * start to draw a path
 * @param p0 start point
 */
void path_begin(float2 p0) noexcept;

/**
 * path line
 * @param p1
 */
void path_line_to(float2 p1) noexcept;

/**
 * path arc
 * @param center
 * @param p1
 * @param ccw whether counter clockwise
 */
void path_arc_to(float2 center, float2 p1, bool ccw) noexcept;

/**
 * path bezier quad draw
 * @param p1
 * @param p2
 */
void path_quad_bezier_to(float2 p1, float2 p2) noexcept;

/**
 * path bezier cubic draw
 * @param p1
 * @param p2
 * @param p3
 */
void path_cubic_bezier_to(float2 p1, float2 p2, float2 p3) noexcept;

/**
 * path draw end
 * @param close close with a line
 * @param color
 * @param thickness
 */
void path_end(bool close = true, Color color = {}, float thickness = {}) noexcept;

/// start collecting closed shapes for a union boolean operation
void union_beg() noexcept;

/**
 * start collecting closed shapes for a cached filled union
 * @param cache_id stable ID for this logical union within the current window
 */
void union_beg(StringID cache_id) noexcept;

/**
 * draw union result of collected closed shapes
 * @param color
 * @param thickness
 */
void union_end(Color color = {}, float thickness = {}) noexcept;

}
