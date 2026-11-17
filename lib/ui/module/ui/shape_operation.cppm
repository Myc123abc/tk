module;

#include <functional>

export module tk.ui:shape_operation;

import :transform;
import tk.string_id;

export namespace tk::ui {

/**
 * begin a transform scope for following draw commands
 * @param transform affine transform applied in draw order
 */
void transform_beg(Matrix const& transform) noexcept;

/**
 * begin a transform scope for following draw commands
 * @param transform transform builder
 */
void transform_beg(Transform const& transform) noexcept;

/// end current transform scope
void transform_end() noexcept;

/**
 * discard the pixel of specific shapes
 * @param func function of discard shapes
 */
void discard_beg(std::function<void()> func) noexcept;

/// discard operation over
void discard_end() noexcept;

/**
 * Start a scissor area.
 * @param pos
 * @param extent
 */
void scissor_area_begin(float2 pos, float2 extent) noexcept;

// End a scissor area
void scissor_area_end() noexcept;

/**
 * Start a viewport which can be dragged with the left mouse button and scaled
 * around the cursor with the mouse wheel. Nested viewport bounds are expressed
 * in the parent viewport's content coordinates.
 * @param name
 * @param pos
 * @param extent
 */
void viewport_begin(StringID name, float2 pos, float2 extent) noexcept;

// End the viewport
void viewport_end() noexcept;

}
