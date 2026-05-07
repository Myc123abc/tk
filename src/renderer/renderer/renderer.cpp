#include "renderer.hpp"
#include "../core.hpp"
#include "../engine/graphics_engine.hpp"
#include "../engine/compute_engine.hpp"
#include "../engine/copy_engine.hpp"
#include "util/error_handling.hpp"
#include "../resource/descriptor_heap_manager.hpp"
#include "pipeline/pipeline_system.hpp"
#include "../window/window_manager.hpp"
#include "context.hpp"
#include "../resource/shader_type.hpp"
#include "../../ui/ui_context.hpp"

#include <dwmapi.h>

#include <stb_image.h>

#include <ranges>

namespace tk::renderer {

void Renderer::init() noexcept
{
  g_core.init();
  g_pipe_sys.init();
  g_desc_heap_mgr.init();
  g_graphics_engine.init();
  g_comp_engine.init();
  g_copy_engine.init(); 
}

void Renderer::render() noexcept
{
  message_process();
  process_render();
}

void Renderer::destroy() noexcept
{
  // pop all message
  while (!_frame_render_complete_funcs.empty())
    message_process();

  // destroy render resources
  g_graphics_engine.destroy();
  g_comp_engine.destroy();
  g_copy_engine.destroy();
  for (auto& res : _res | std::views::values) res.destroy();
}

void Renderer::create_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept
{
  err_if(_res.contains(handle), "failed to create window render resource, it's already exist");
  auto res = RenderResource{};
  res.init(handle, width, height);
  _res.emplace(handle, std::move(res));
}

void Renderer::destroy_window_resource(HWND handle, HWND blur_handle) noexcept
{
  err_if(!_res.contains(handle), "failed to destroy window render resource, it's unexist");
  add_frame_render_complete_func([handle = handle, blur_handle = blur_handle, res = std::move(_res[handle])] mutable
  {
    res.destroy();
    g_wnd_mgr.destroy_window(handle, blur_handle);
  });
  _res.erase(handle);
  _destroied_windows.emplace(handle);
}

void Renderer::resize_window_resource(HWND handle, uint32_t width, uint32_t height) noexcept
{
  err_if(!_res.contains(handle), "failed to destroy window render resource, it's unexist");
  _res[handle].resize(width, height);
  ui::g_ui_ctx.get_window_context(handle)->frame_data()->init(width, height);
}

void Renderer::create_image(ui::ImageHandle handle, uint32_t width, uint32_t height, ImageFormat format) noexcept
{
  auto image = Image{};
  image.init(width, height, format, ImageType::srv);

  assert(!_images.contains(handle));
  _images.emplace(handle, std::move(image));
}

void Renderer::destroy_image(ui::ImageHandle handle) noexcept
{
  if (_upload_images.contains(handle))
  {
    assert(_bitmaps.contains(handle));

    // not upload yet, remove upload image
    _upload_images.erase(handle);
    _bitmaps.erase(handle);
  }
  else if (_images.contains(handle))
  {
    // already uploaded, release image resource
    add_frame_render_complete_func([image = std::move(_images[handle])] mutable { image.destroy(); });
    _images.erase(handle);
  }
}

void Renderer::upload_image(ui::ImageHandle handle, uint32_t width, uint32_t height, Bitmap const& bitmap, bool use_mipmap) noexcept
{
  // create image resource
  auto image = Image{};
  image.init(bitmap.width, bitmap.height, ImageFormat::rgba8_unorm, ImageType::srv, use_mipmap);

  // store image and bitmap for upload
  _upload_images[handle] = std::move(image);
  _bitmaps[handle]       = bitmap;

  if (use_mipmap)
    _pending_mipmap_image_handles.emplace_back(handle);
}

void Renderer::add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept
{
  auto last_fence_values = std::vector<std::pair<Engine&, uint64_t>>
  {
    { g_graphics_engine, g_graphics_engine.signal() },
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
}

void Renderer::preprocess_render() noexcept
{
  upload_images();
  g_comp_engine.update();

  if (_blur_host_window)
  {
    DwmFlush();
    g_wnd_mgr.resize_blur_window(_blur_host_window, _blur_window_rect);
    clear_blur_resize_data();
  }
}

void Renderer::upload_images() noexcept
{
  if (!_upload_images.empty())
  {
    assert(_upload_images.size() == _bitmaps.size());
    
    // upload images by copy engine
    g_copy_engine.acquire_slot();
    g_copy_engine.copy(
      _bitmaps
        | std::views::values
        | std::ranges::to<std::vector<Bitmap>>(),
      _upload_images
        | std::views::values
        | std::views::transform([](auto& img) { return &img; })
        | std::ranges::to<std::vector<Image*>>());
    auto fence_value = g_copy_engine.submit_slot();

    // wait upload images complete before rendering
    g_graphics_engine.wait(g_copy_engine, fence_value);
    // also wait for compute engine if need to generate mipamp
    if (!_pending_mipmap_image_handles.empty())
      g_comp_engine.wait(g_copy_engine, fence_value);

    // move upload images to images
    for (auto& [idx, img] : _upload_images)
      _images[idx] = std::move(img);

    // free bitmaps
    for (auto const& bitmap : _bitmaps | std::views::values) stbi_image_free(bitmap.data);

    _upload_images.clear();
    _bitmaps.clear();
  }
}

void Renderer::process_render() noexcept
{  
  preprocess_render();

  generate_mipmap();

  for (auto [handle, data, blur_host_window, blur_window_rect] : _render_infos)
  {
    // continue if the window is destoried
    if (_destroied_windows.contains(handle)) continue;
    _render_windows.emplace_back(handle);

    // render
    auto& res = _res[handle];
    res.wait_frame_complete();
    res.render_begin();
    render(res, data);
    res.render_end();

    if (blur_host_window)
    {
      _blur_host_window = blur_host_window;
      _blur_window_rect = blur_window_rect;
    }
  }
  _render_infos.clear();

  // present windows
  if (_render_windows.size() == 1)
    _res[_render_windows.back()].present(true);
  else if (_render_windows.size() > 1)
  {
    for (auto handle : _render_windows | std::views::take(_render_windows.size() - 1))
      _res[handle].present(false);
    _res[_render_windows.back()].present(true);
  }

  // show blur window
  if (!_show_blur_wnds.empty() &&
      std::ranges::any_of(_render_windows, [&](auto handle) { return _show_blur_wnds.contains(handle); }))
  {
    DwmFlush();
    for (auto wnd : _show_blur_wnds | std::views::keys)
      g_wnd_mgr.get_window(wnd)->show_blur_window();
    _show_blur_wnds.clear();
  }

  postprocess_render();
}

void Renderer::render(RenderResource& res, ui::FrameData const* frame_data) noexcept
{
  if (!frame_data) return;

  auto cmd = g_graphics_engine.cmd();

  g_ctx.set_cmd(cmd);

  auto& frame = res.current_frame();
  frame.buffer.clear().upload(cmd, frame_data);

  for (auto const& call : frame_data->render_calls())
  {
    using Type = ui::RenderCallType;
    switch (call.type)
    {
      case Type::ui:
        g_ctx.graphics_pipe_set(PipelineType::ui, call.scissor_rect, "constants", Constants
        {
          .render_target_extent = res.render_target()->extent(),
          .window_extent        = frame_data->window_extent(),
          .window_pos           = call.window_pos,
          .tile_size            = frame_data->tile_size(),
          .tile_count           = frame_data->tile_count(),
        },
        {
          { "cmds",      frame.buffer.cmds_gpu_handle()                                    },
          { "path_cmds", frame.buffer.path_cmds_gpu_handle()                               },
          { "cmd_idxs",  frame.buffer.cmd_idxs_gpu_handle()                                },
          { "tiles",     frame.buffer.tiles_gpu_handle( )                                  },
          { "images",    g_desc_heap_mgr.first_gpu_handle(DescriptorHeapType::cbv_srv_uav) },
        });
        g_ctx.draw(2);
        break;

      case Type::window_shadow:
        g_ctx.graphics_pipe_set(PipelineType::window_shadow, call.scissor_rect, "constants", Constants
        {
          .render_target_extent = res.render_target()->extent(),
          .window_extent        = call.window_shadow.window_extent,
          .window_pos           = call.window_pos,
          .shadow_thickness     = call.window_shadow.shadow_thickness,
          .shadow_radius        = call.window_shadow.radius,
          .shadow_color         = call.window_shadow.color,
          .shadow_softness      = call.window_shadow.softness,
          .wireframe_color      = call.window_shadow.wireframe_color ? call.window_shadow.wireframe_color.value() : float4{},
          .draw_wireframe       = call.window_shadow.wireframe_color.has_value(),
        });
        g_ctx.draw(2);
        break;
    }
  }
}

void Renderer::postprocess_render() noexcept
{
  _destroied_windows.clear();
  _render_windows.clear();
}

void Renderer::generate_mipmap() noexcept
{
#if 0
  if (_pending_mipmap_image_handles.empty()) return;

  g_comp_engine.acquire_slot();

  auto cmd = g_comp_engine.cmd();
  _mipmap_pipeline.bind(cmd);

  // generate mipmaps of images
  struct MipmapGenerationCostant
  {
    float2 texel_size{};
    uint32_t  mip_level{};
  };
  auto constants = MipmapGenerationCostant{};
  for (auto const& handle : _pending_mipmap_image_handles)
  {
    auto const& img = _images[handle];

    auto src_width  = img.width();
    auto src_height = img.height();

    for (auto i : std::views::iota(0u, img.mipmap_uavs().size()))
    {
      constants.texel_size = float2{ 1.0 / src_width, 1.0 / src_height };
      constants.mip_level  = i;

      _mipmap_pipeline.set_constants(cmd, "constants", constants);
      _mipmap_pipeline.set_descriptors(cmd,
      {
        { "src", img.gpu_handle()                  },
        { "dst", img.mipmap_uavs()[i].gpu_handle() },
      });

      auto dst_width  = std::max(1u, src_width  >> 1);
      auto dst_height = std::max(1u, src_height >> 1);

      cmd->Dispatch(((dst_width + 7) / 8), ((dst_height + 7) / 8), 1);

      src_width  = dst_width;
      src_height = dst_height;
    }
  }

  // wait mipmap generation complete
  g_graphics_engine.wait(g_comp_engine, g_comp_engine.submit_slot());

  // release mipmap uavs after generation complete
  add_frame_render_complete_func([this, handles = std::move(_pending_mipmap_image_handles)]
  {
    for (auto const& handle : handles)
      _images[handle].release_mipmap_uavs();
  });
  _pending_mipmap_image_handles.clear();
#endif
}

}
