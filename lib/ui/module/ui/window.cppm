module;

#include <optional>
#include <functional>

export module tk.ui:window;

import tk.ui.color;
import tk.ui.window_config;
import tk.base;
import tk.string_id;

export namespace tk::ui {

/**
 * get cursor position
 * @return cursor position
 */
auto get_cursor_pos() noexcept -> float2;

/**
 * get cursor position on window
 * @return cursor position
 */
auto get_cursor_pos_on_window() noexcept -> float2;

/**
 * begin a window
 * @param name window name cannot be duplicated
 * @param extent window client extent; controls determine it when omitted
 * @param pos window position; the system chooses it when omitted
 * @param is_closed make the window can be closed if is_closed is not nullptr,
 *                  and this is only a flag to indicate whether the window is closed,
 *                  if you want to close the window, stop call the begin and end of this window
 * @param cfg window config
 */
void begin(StringID name, std::optional<float2> extent = {}, std::optional<float2> pos = {}, bool* is_closed = {}, WindowConfig const& cfg = {}) noexcept;

// end a window
void end() noexcept;

/**
 * Draw on the current window's title bar during end().
 * The callback origin is the title bar's left-top corner.
 * @param func receives the drawable title bar extent excluding window controls
 */
void draw_on_title_bar(std::move_only_function<void(float2)>&& func) noexcept;

/**
 * set cursor move invalid area
 * every frame rendering will be reset
 * @param x
 * @param y
 * @param width
 * @param height
 */
void add_move_invalid_area(float2 left_top, float2 right_bottom) noexcept;

/**
 * get window extent in current update function of window
 * @return extent of window
 */
auto window_extent() noexcept -> float2;

/**
 * get window position in current update function of window
 * @return position of window
 */
auto window_pos() noexcept -> float2;

/**
 * get window extent in current update function of window
 * @return extent of window without titlebar
 */
auto window_drawable_extent() noexcept -> float2;

/**
 * whether current window is maximized
 * @return is window fullscreen
 */
auto is_maximized_window() noexcept -> bool;

// maximize window
void maximize_window() noexcept;

/**
 * whether current window is fullscreen
 * @return is window fullscreen
 */
auto is_fullscreen_window() noexcept -> bool;

// fullscreen window
void fullscreen_window() noexcept;

// restore window if fullscreen
void restore_fullscreen_window() noexcept;

}
