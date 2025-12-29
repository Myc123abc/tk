#include "ui/ui.hpp"
#include "ui_context.hpp"

using namespace tk::renderer;

namespace tk { namespace ui {

////////////////////////////////////////////////////////////////////////////////
///                              Misc
////////////////////////////////////////////////////////////////////////////////

void render() noexcept
{
	g_ui_ctx.render();
}

////////////////////////////////////////////////////////////////////////////////
///                             Window
////////////////////////////////////////////////////////////////////////////////

void begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed) noexcept
{
	g_ui_ctx.begin(name, x, y, width, height, is_closed);
}

void end() noexcept
{
	g_ui_ctx.end();
}

////////////////////////////////////////////////////////////////////////////////
///                            Geometry
////////////////////////////////////////////////////////////////////////////////

void rectangle(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
{
	g_ui_ctx.check_draw();
	// TODO: check_not_path_draw
	// TODO: window_render_pos offset

	g_ui_ctx.add_shape(ShapeProperty::Type::rectangle, color, thickness, { left_top.x, left_top.y, right_bottom.x, right_bottom.y }, { left_top, right_bottom });
}

}}
