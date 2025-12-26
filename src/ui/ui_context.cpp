#include "ui_context.hpp"
#include "../util/error_handling.hpp"
#include "../renderer/window/window_manager.hpp"
#include "../renderer/renderer.hpp"

using namespace tk::renderer;

namespace tk { namespace ui {

void UIContext::begin(std::string_view name, int x, int y, uint32_t width, uint32_t height, bool* is_closed) noexcept
{
  err_if(_call_begin, "begin is called but end not be called");
  _call_begin = true;

  // create window if not have
  if (!_windows.contains(name.data()))
  {
    auto handle = g_wnd_mgr.create_window(x, y, width, height);
    _window = &_windows[name.data()];
    _window->handle        = handle;
    _window->can_be_closed = is_closed;
  }

  _window = &_windows[name.data()];
  _window->is_called = true;
  if (is_closed)
    *is_closed = _window->is_closed;

  _shape_properties_offset = 0;
  _idx_beg                 = 0;
}

void UIContext::end() noexcept
{
  err_if(!_call_begin, "begin is not called but end is called");
  _call_begin = false;
}

void UIContext::check_draw() const noexcept
{
  err_if(!_call_begin, "not called begin to draw something");
}

void UIContext::render() noexcept
{
  for (auto& [_, window] : _windows)
  {
    if (window.is_called)
    {
      // render
      auto render_data = window.data();
      if (!render_data->empty() && !render_data->is_using())
      {
        if (g_renderer.render(window.handle, window.data()))
          window.next_frame();
      }
      window.no_frame_can_use = render_data->is_using();
      window.is_called = false;
    }
    else
    {
      // destroy
      // TODO: begin not be called again, destroy the window
      //       so it shouldn't be destroy be close window
      //       only notify user the window can be closed when close window
      //       then user use the flag to decide whether not render window again
    }
  }

  _msg_queue.process(MessageHandler{});
}

void UIContext::add_vertices_indices(std::pair<glm::vec2, glm::vec2> const& bounding_rectangle) noexcept
{
  auto render_data = _window->data();
  auto [min, max]  = bounding_rectangle;

	// TODO:
  // auto offset = ctx->op_data.op == ShapeProperty::Operator::none ? ctx->shape_properties_offset : ctx->op_data.offset;
	auto offset = _shape_properties_offset;

  render_data->vertices.append_range(std::vector<Vertex>
  {
    { { min.x, min.y, 0.f }, { 0.f, 0.f }, offset },
    { { max.x, min.y, 0.f }, { 1.f, 0.f }, offset },
    { { max.x, max.y, 0.f }, { 1.f, 1.f }, offset },
    { { min.x, max.y, 0.f }, { 0.f, 1.f }, offset },
  });
  render_data->indices.append_range(std::vector<uint16_t>
  {
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 1),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 0),
    static_cast<uint16_t>(_idx_beg + 2),
    static_cast<uint16_t>(_idx_beg + 3),
  });
  _idx_beg += 4;
}

void UIContext::add_shape_property(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values) noexcept
{
  auto render_data = _window->data();
  render_data->shape_properties.emplace_back(ShapeProperty
  {
    type,
    // TODO:
    // ctx->tmp_color.value_or(color),
    color,
    thickness,
    // TODO:
    // ctx->op_data.op,
    ShapeProperty::Operator::none,
    values
  });
  _shape_properties_offset += render_data->shape_properties.back().byte_size();
}

void UIContext::add_shape(renderer::ShapeProperty::Type type, glm::vec4 color, float thickness, std::vector<float> const& values, std::pair<glm::vec2, glm::vec2> const& bounding_rectangle) noexcept
{
  auto [min, max] = bounding_rectangle;

  // if (ctx->op_data.op == ShapeProperty::Operator::u)
  // {
  //   ctx->op_data.points.emplace_back(min);
  //   ctx->op_data.points.emplace_back(max);
  //   goto add_shape_property;
  // }

  add_vertices_indices(bounding_rectangle);

add_shape_property:
  add_shape_property(type, color, thickness, values);
}

}}
