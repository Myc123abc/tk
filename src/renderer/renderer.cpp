#include "renderer.hpp"
#include "core.hpp"
#include "engine/graphics_engine.hpp"
#include "engine/copy_engine.hpp"
#include "util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"
#include "compiler.hpp"

#include <ranges>

namespace tk { namespace renderer {

void Renderer::init() noexcept
{
  _thread = std::jthread([this]
  {
    g_core.init();
    g_compiler.init();
    g_desc_heap_mgr.init();
    g_graphics_engine.init();
    g_copy_engine.init();

    _sdf_pipeline.init_graphics("assets/shader/sdf.hlsl", "vs", "ps", "assets/shader", RenderResource::Render_Target_Format, true);

    while (!_exit.load(std::memory_order_relaxed))
    {
      message_process();
      if (!_render_datas.empty())
      {
        render();
        _frame_sem.release();
      }
      else
        _render_data_empty_sem.acquire();
    }
  });
}

void Renderer::destroy() noexcept
{
  // exit render loop
  _exit.store(true, std::memory_order_relaxed);
  _frame_sem.release();
  _render_data_empty_sem.release();
  _thread.join();

  // pop all message
  while (!_frame_render_complete_funcs.empty() || !_msg_queue.empty())
    message_process();

  // destroy render resources
  g_graphics_engine.destroy();
  g_copy_engine.destroy();
  for (auto& res : _res | std::views::values) res.destroy();
}

void Renderer::add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept
{
  auto last_fence_values = std::vector<std::pair<Engine&, uint64_t>>
  {
    { g_graphics_engine, g_graphics_engine.signal() },
    // TODO: need this?
    // { g_copy_engine,     g_copy_engine.signal() },
  };
  _frame_render_complete_funcs.emplace_back([func = std::move(func), last_fence_values = std::move(last_fence_values)]() mutable
  {
    for (auto [engine, last_fence_value] : last_fence_values)
    {
      auto fence_value = engine.fence_completed_value();
      err_if(fence_value == UINT64_MAX, "failed to get fence value because device is removed");
      if (fence_value < last_fence_value) return false;
    }
    func();
    return true;
  });
}

void Renderer::message_process() noexcept
{
  for (auto it = _frame_render_complete_funcs.begin(); it != _frame_render_complete_funcs.end();)
    (*it)() ? it = _frame_render_complete_funcs.erase(it) : ++it;

  _msg_queue.process(MessageHandler{ g_renderer });
}

void Renderer::render() noexcept
{
  for (auto _ : std::views::iota(0u, _render_datas.size()))
  {
    // pop render data
    auto [handle, render_data] = *_render_datas.front();
    _render_datas.pop();

    // continue if the window is destoried
    if (_destroied_windows.contains(handle)) continue;
    _rendered_windows.emplace_back(handle);

    // promise window is valid
    err_if(!_res.contains(handle), "failed to render. No render resource exist on handle {}", (size_t)handle);

    // last frame is complete, rendering, otherwise, discard
    auto& res = _res[handle];
    res.wait_frame_complete();
    res.render_begin();
    render_sdf(res, render_data->vertices, render_data->indices, render_data->shape_properties);
    res.render_end();

    render_data->clear();
  }

  // present windows
  if (_rendered_windows.size() == 1)
    _res[_rendered_windows.back()].present(true);
  else if (_rendered_windows.size() > 1)
  {
    for (auto handle : _rendered_windows | std::views::take(_rendered_windows.size() - 1))
      _res[handle].present(false);
    _res[_rendered_windows.back()].present(true);
  }

  _destroied_windows.clear();
  _rendered_windows.clear();
}

void Renderer::render_sdf(RenderResource& res, std::span<Vertex const> vertices, std::span<uint16_t const> indices, std::span<ShapeProperty const> shape_properties) noexcept
{
  auto& frame               = res.current_frame();
  auto& render_target_image = g_image_pool[frame.image];
  auto  cmd                 = res.graphics_cmd();

  // bind pipeline
  _sdf_pipeline.bind(cmd);

  // upload data to buffer
  frame.buffer.clear().upload(cmd, vertices, indices, shape_properties);

  // set descriptors
  auto constants = Constants{};
  constants.window_extent = render_target_image.extent();
  // constants.window_pos    = window.content_pos();
  // if (fullscreen_target_window.has_value())
  // {
  //   constants.window_pos   = fullscreen_target_window->pos();
  //   constants.cursor_index = g_image_pool[renderer->_cursors[fullscreen_target_window->cursor_type].handle].index();
  // }
  _sdf_pipeline.set_descriptors(cmd, "constants", constants,
  {
    // { "images", g_desc_heap_mgr.first_gpu_handle(DescriptorHeapType::cbv_srv_uav) },
    { "buffer", frame.buffer.gpu_handle()                                         },
  });

  // draw
  // if (fullscreen_target_window.has_value())
  // {
  //   cmd->RSSetScissorRects(1, &fullscreen_target_window->rect);
  //   cmd->DrawIndexedInstanced(indices.size() - 6, 1, 0, 0, 0);
  //   auto rect = window.real_rect();
  //   cmd->RSSetScissorRects(1, &rect);
  //   cmd->DrawIndexedInstanced(6, 1, indices.size() - 6, 0, 0);
  // }
  // else
  {
    auto rect = RECT{};
    // rect.left   = Window_Shadow_Thickness;
    // rect.top    = Window_Shadow_Thickness;
    // rect.right  = rect.left + window.width;
    // rect.bottom = rect.top  + window.height;
    rect.right  = constants.window_extent.x;
    rect.bottom = constants.window_extent.y;
    cmd->RSSetScissorRects(1, &rect);
    cmd->DrawIndexedInstanced(indices.size(), 1, 0, 0, 0);
  }
}

}}
