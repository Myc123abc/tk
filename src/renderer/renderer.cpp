#include "renderer.hpp"
#include "core.hpp"
#include "engine/graphics_engine.hpp"
#include "engine/compute_engine.hpp"
#include "engine/copy_engine.hpp"
#include "util/error_handling.hpp"
#include "resource/descriptor_heap_manager.hpp"
#include "pipeline/pipeline_system.hpp"
#include "window/window_manager.hpp"
#include "context.hpp"
#include "resource/shader_type.hpp"
#include "config.hpp"

#include <dwmapi.h>

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

  init_images();  
}

void Renderer::init_images() noexcept
{
  _discard_image   = g_img_mgr.create(Default_Image_Init_Width, Default_Image_Init_Height, ImageFormat::r8_unorm,                ImageType::rtv | ImageType::srv);
  _composite_image = g_img_mgr.create(Default_Image_Init_Width, Default_Image_Init_Height, RenderResource::Render_Target_Format, ImageType::rtv | ImageType::srv);
}

void Renderer::destroy_images() noexcept
{
  g_img_mgr.destroy(_discard_image);
  g_img_mgr.destroy(_composite_image);
}

void Renderer::render() noexcept
{
  message_process();
  process_render();
}

void Renderer::wait_idle() noexcept
{
  g_graphics_engine.wait_idle();
  g_copy_engine.wait_idle();
  g_comp_engine.wait_idle();
}

void Renderer::destroy() noexcept
{
  push_tmp_frame_render_complete_funcs();
  while (!_frame_render_complete_funcs.empty())
    message_process();

  destroy_images();

  // destroy render resources
  for (auto& res : _res | std::views::values) res.destroy();

  g_img_mgr.destroy();

  g_graphics_engine.destroy();
  g_comp_engine.destroy();
  g_copy_engine.destroy();
}

void Renderer::create_window_resource(HWND handle, uint width, uint height) noexcept
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

void Renderer::resize_window_resource(HWND handle, uint width, uint height) noexcept
{
  err_if(!_res.contains(handle), "failed to destroy window render resource, it's unexist");
  _res[handle].resize(width, height);
}

void Renderer::add_frame_render_complete_func(std::move_only_function<void()>&& func) noexcept
{
  _tmp_frame_render_complete_funcs.emplace_back(std::move(func));
}

void Renderer::message_process() noexcept
{
  for (auto it = _frame_render_complete_funcs.begin(); it != _frame_render_complete_funcs.end();)
    (*it)() ? it = _frame_render_complete_funcs.erase(it) : ++it;
}

void Renderer::preprocess_render() noexcept
{
  g_copy_engine.update();
  g_comp_engine.update();

  if (_blur_host_window)
  {
    DwmFlush();
    g_wnd_mgr.resize_blur_window(_blur_host_window, _blur_window_rect);
    clear_blur_resize_data();
  }
}

void Renderer::process_render() noexcept
{  
  preprocess_render();

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

  assert(frame_data->check());

  auto cmd = g_graphics_engine.cmd();

  g_ctx.set_cmd(cmd);

  res.current_frame().buffer.clear().upload(cmd, frame_data);

  auto& rt_img = g_img_mgr[res.render_target()];

  auto constants = Constants{};

  auto clear_resize_render_target = [this, cmd](ImageHandle& handle, Rect rect)
  {
    auto& img = g_img_mgr[handle];
    auto required_width  = static_cast<uint>(std::ceil(rect.width()));
    auto required_height = static_cast<uint>(std::ceil(rect.height()));
    if (required_width > img.width() || required_height > img.height())
    {
      auto width  = std::max(required_width,  img.width());
      auto height = std::max(required_height, img.height());
      add_frame_render_complete_func([handle] { g_img_mgr.destroy(handle); });
      handle = g_img_mgr.create(width, height, img);
    }
    cmd->clear_render_target(handle);;
  };

  auto graphics_draw = [&](ui::RenderCmd const& cmd, PipelineType type, ImageHandle rt, ImageHandle ds, Rect scissor, std::vector<PipelineDescriptorInfo> const& descs = {})
  {
    auto& rt_img = g_img_mgr[rt];
    constants.render_target_extent = rt_img.extent();
    g_ctx.graphics_draw(GraphicsDrawInfo
    {
      .pipe_info = GraphicsPipeSetInfo
      {
        .type           = type,
        .viewport       = rt_img.rect(),
        .scissor        = scissor,
        .constants_name = "constants",
        .constants      = constants,
        .descs          = descs,
      },
      .render_target = rt,
      .depth_stencil = ds,
      .idx_beg       = cmd.idx_beg,
      .idx_cnt       = cmd.idx_size,
    });
  };

  Rect discard_mask_rc, discard_composite_rc;

  auto rt = res.render_target();
  auto ds = res.depth_stencil();

  for (auto const& render_cmd : frame_data->render_cmds())
  {
    using Type = ui::RenderCmdType;
    switch (render_cmd.type)
    {
    case Type::ui:
      constants.window_pos       = frame_data->window_pos();
      constants.composite_offset = {};
      graphics_draw(render_cmd, PipelineType::ui, rt, {}, render_cmd.rect,
      {
        { "images", g_desc_heap_mgr.first_gpu_handle(DescriptorHeapType::cbv_srv_uav) },
      });
      break;

    case Type::clear_discard_image:
      discard_mask_rc = render_cmd.rect;
      clear_resize_render_target(_discard_image, discard_mask_rc);
      break;

    case Type::clear_composite_image:
      discard_composite_rc = render_cmd.rect;
      clear_resize_render_target(_composite_image, discard_composite_rc);
      break;

    case Type::discard_write:
      constants.mask_offset = -discard_mask_rc.pos();
      graphics_draw(render_cmd, PipelineType::mask_write_max, _discard_image, {}, { 0, 0, discard_mask_rc.extent() });
      break;

    case Type::discard_draw_ui_composite:
      constants.window_pos       = {};
      constants.composite_offset = -discard_composite_rc.pos();
      graphics_draw(render_cmd, PipelineType::ui, _composite_image, {}, { 0, 0, discard_composite_rc.extent() },
      {
        { "images", g_desc_heap_mgr.first_gpu_handle(DescriptorHeapType::cbv_srv_uav) },
      });
      break;

    case Type::discard_composite:
    {
      auto& discard_img   = g_img_mgr[_discard_image];
      auto& composite_img = g_img_mgr[_composite_image];

      constants.window_pos       = frame_data->window_pos();
      constants.mask_offset      = -discard_mask_rc.pos();
      constants.mask_extent      = discard_img.extent();
      constants.composite_offset = -discard_composite_rc.pos();
      constants.composite_extent = composite_img.extent();

      cmd->transform({
        { _discard_image,   ImageState::pixel },
        { _composite_image, ImageState::pixel },
      });

      graphics_draw(render_cmd, PipelineType::discard_draw, rt, {}, render_cmd.rect,
      {
        { "mask_image",      discard_img.srv().gpu_handle()   },
        { "composite_image", composite_img.srv().gpu_handle() },
      });
    }
    break;
    }
  }

  auto const& window_shadow_info = frame_data->window_shadow_info();
  if (window_shadow_info)
  {
    g_ctx.set_render_target(rt, {});
    g_ctx.graphics_pipe_set(GraphicsPipeSetInfo
    {
      .type           = PipelineType::window_shadow,
      .viewport       = rt_img.rect(),
      .scissor        = window_shadow_info->scissor_rect,
      .constants_name = "constants",
      .constants      = Constants
      {
        .render_target_extent = rt_img.extent(),
        .window_extent        = window_shadow_info->window_extent,
        .window_pos           = frame_data->window_pos(),
        .shadow_thickness     = window_shadow_info->shadow_thickness,
        .shadow_radius        = window_shadow_info->radius,
        .shadow_color         = window_shadow_info->color,
        .shadow_softness      = window_shadow_info->softness,
        .wireframe_color      = window_shadow_info->wireframe_color ? window_shadow_info->wireframe_color.value() : float4{},
        .draw_wireframe       = window_shadow_info->wireframe_color.has_value(),
      }
    });
    g_ctx.draw(2);
  }
}

void Renderer::postprocess_render() noexcept
{
  _destroied_windows.clear();
  _render_windows.clear();
  push_tmp_frame_render_complete_funcs();
}

void Renderer::push_tmp_frame_render_complete_funcs() noexcept
{
  if (_tmp_frame_render_complete_funcs.empty()) return;

  auto last_fence_values = std::vector<std::pair<Engine&, uint64>>
  {
    { g_graphics_engine, g_graphics_engine.signal() },
    { g_copy_engine,     g_copy_engine.signal()     },
    { g_comp_engine,     g_comp_engine.signal()     },
  };

  _frame_render_complete_funcs.emplace_back([funcs = std::move(_tmp_frame_render_complete_funcs), last_fence_values = std::move(last_fence_values)]() mutable
  {
    for (auto [engine, last_fence_value] : last_fence_values)
    {
      auto fence_value = engine.fence_completed_value();
      if (fence_value == UINT64_MAX)
      {
        auto reason = g_core.device()->GetDeviceRemovedReason();
        err_if(true, "failed to get fence value because device is removed: 0x{:08x}", static_cast<uint>(reason));
      }
      if (fence_value < last_fence_value) return false;
    }
    for (auto& func : funcs) func();
    return true;
  });
}

// void copy(
//   Command const*   cmd,
//   ImageHandle      src,
//   LONG             left,
//   LONG             top,
//   LONG             right,
//   LONG             bottom,
//   ID3D12Resource*  readback_buffer,
//   uint sub) noexcept
// {
//   auto& img = g_img_mgr[src];

//   img.transform(cmd, ImageState::copy_src);
//   auto src_loc    = CD3DX12_TEXTURE_COPY_LOCATION{ img.handle(), sub };
//   auto region_box = CD3DX12_BOX{ left, top, right, bottom };

//   auto footprint = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{};
//   footprint.Footprint.Width    = right - left;
//   footprint.Footprint.Height   = bottom - top;
//   footprint.Footprint.Depth    = 1;
//   footprint.Footprint.RowPitch = align(img.per_pixel_size() * footprint.Footprint.Width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
//   footprint.Footprint.Format   = static_cast<DXGI_FORMAT>(img.format());
//   auto dst_loc = CD3DX12_TEXTURE_COPY_LOCATION{ readback_buffer, footprint };

//   cmd->get()->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, &region_box);
// }

// auto Renderer::readback(Command const* cmd, ImageHandle img_h, uint sub) noexcept -> ReadbackResult
// {
//   auto& img = g_img_mgr[img_h];
//   err_if(img.per_pixel_size() != 4, "readback only support rgba image now");

//   auto rect = img.rect();
//   rect.right  = static_cast<uint>(rect.right)  >> sub;
//   rect.bottom = static_cast<uint>(rect.bottom) >> sub;
//   auto left = std::max(rect.left, 0.f);
//   auto top  = std::max(rect.top, 0.f);

//   // create bitmap view
//   auto view = Bitmap{};
//   view.x      = left;
//   view.y      = top;
//   view.width  = rect.right  - view.x;
//   view.height = rect.bottom - view.y;

//   // create readback buffer
//   auto readback_buffer = ComPtr<ID3D12Resource>{};
//   auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
//   view.row_pitch       = align(view.width * img.per_pixel_size(), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
//   auto heap_desc       = CD3DX12_RESOURCE_DESC::Buffer(align(view.row_pitch * view.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
//   err_if(g_core.device()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &heap_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback_buffer)),
//           "failed to create readback buffer");

//   // get pointer of readback buffer
//   auto range = D3D12_RANGE{ 0, heap_desc.Width };
//   err_if(readback_buffer->Map(0, &range, reinterpret_cast<void**>(&view.data)), "failed to map readback buffer to pointer");

//   // copy data from gpu to cpu
//   copy(cmd, img_h, view.x, view.y, rect.right, rect.bottom, readback_buffer.Get(), sub);

//   return ReadbackResult{ readback_buffer, view };
// }

}
