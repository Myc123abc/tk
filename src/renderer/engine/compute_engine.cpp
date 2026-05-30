#include "compute_engine.hpp"
#include "../resource/descriptor_heap_manager.hpp"
#include "../pipeline/pipeline_system.hpp"
#include "../resource/shader_type.hpp"
#include "graphics_engine.hpp"
#include "../context.hpp"
#include "../renderer.hpp"

#include <assert.h>

using namespace tk::renderer;

namespace {

auto get_gauss_weights(float sigma) noexcept
{
  sigma = fmin(sigma, Max_Blur_Widget_Num / 2.f);

  float twoSigma2 = 2.0f*sigma*sigma;

  // Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
  // For example, for sigma = 3, the width of the bell curve is 
  int blurRadius = (int)ceil(2.0f * sigma);

  std::vector<float> weights;
  weights.resize(2 * blurRadius + 1);

  float weightSum = 0.0f;

  for(int i = -blurRadius; i <= blurRadius; ++i)
  {
    float x = (float)i;

    weights[i + blurRadius] = expf(-x*x / twoSigma2);

    weightSum += weights[i + blurRadius];
  }

  // Divide by the sum so all the weights add up to 1.0.
  for(int i = 0; i < weights.size(); ++i)
  {
    weights[i] /= weightSum;
  }

  return weights;
}

}

namespace tk::renderer {

void ComputeEngine::destroy() noexcept
{
  for (auto const& img : _blur_tmp_images) g_img_mgr.destroy(img.img);
  Engine::destroy();
}

auto ComputeEngine::get_tmp_img() noexcept -> std::pair<Image*, uint32_t>
{
  // refactoring code
  assert(false);
  Image*   tmp_img{};
  uint32_t idx{};
  if (auto it = std::ranges::find(_blur_tmp_images, false, &BlurTmpImage::in_use);
      it != _blur_tmp_images.end())
  {
    it->in_use = true;
    tmp_img    = g_img_mgr.get(it->img);
    idx        = it - _blur_tmp_images.begin();
  }
  else
  {
    auto& res = _blur_tmp_images.emplace_back();
    res.in_use = true;
    tmp_img    = g_img_mgr.get(res.img);
    idx        = _blur_tmp_images.size() - 1;
  }
  return { tmp_img, idx };
}

void ComputeEngine::blur(Image& src, Image& dst, float sigma, uint32_t blur_count) noexcept
{
  // refactoring code
  assert(false);

  // copy image
  g_graphics_engine.acquire_slot();
  g_graphics_engine.copy(src, dst);
  g_graphics_engine.submit_slot();

  // calculate widgets and blur radius
  auto widgets   = get_gauss_weights(sigma);
  auto constants = BlurConstants{};
  constants.blur_radius = widgets.size() / 2;
  memcpy(&constants.widgets, widgets.data(), widgets.size() * sizeof(float));

  _slots.acquire_slot();
  g_desc_heap_mgr.bind_heaps(cmd());

  auto width  = src.width();
  auto height = src.height();

  // get tmp image
  auto [tmp_img, idx] = get_tmp_img();
  if (!tmp_img->width())
    tmp_img->init(width, height, src.format(), ImageType::srv | ImageType::uav);
  else
    tmp_img->resize(width, height);

  auto const& horizontal_pipe = g_pipe_sys.pipe(PipelineType::blur_horizontal_pass);
  auto const& vertical_pipe   = g_pipe_sys.pipe(PipelineType::blur_vertical_pass);

  g_ctx.set_cmd(cmd());
  g_ctx.set_compute_root_signature(horizontal_pipe->root_signature);
  g_ctx.set_compute_constants(horizontal_pipe->root_param_idx("constants"), constants);

  for (auto i = 0; i < blur_count; ++i)
  {
    g_ctx.set_pipe(horizontal_pipe->pipe_state.Get());
    dst.set_state(cmd(), ImageState::non_pixel);
    g_ctx.set_compute_descriptor(horizontal_pipe->root_param_idx("src"), dst.srv().gpu_handle());
    tmp_img->set_state(cmd(), ImageState::compute_rw);
    g_ctx.set_compute_descriptor(horizontal_pipe->root_param_idx("dst"), tmp_img->uav().gpu_handle());
    g_ctx.dispatch(ceil(width / 128.f), height, 1);

    g_ctx.set_pipe(vertical_pipe->pipe_state.Get());
    tmp_img->set_state(cmd(), ImageState::non_pixel);
    g_ctx.set_compute_descriptor(vertical_pipe->root_param_idx("src"), tmp_img->srv().gpu_handle());
    dst.set_state(cmd(), ImageState::compute_rw);
    g_ctx.set_compute_descriptor(vertical_pipe->root_param_idx("dst"), dst.uav().gpu_handle());
    g_ctx.dispatch(width, ceil(height / 128.f), 1);
  }

  _used_blur_tmp_images.emplace_back(idx, _slots.submit_slot());
}

void ComputeEngine::update() noexcept
{
  generate_mipmaps();

#if 0
  auto finish_value = fence_completed_value();
  for (auto it = _used_blur_tmp_images.begin(); it != _used_blur_tmp_images.end();)
  {
    auto [idx, fence_value] = *it;
    if (fence_value <= finish_value)
    {
      _blur_tmp_images[idx].in_use = false;
      it = _used_blur_tmp_images.erase(it);
      // TODO: only compute engine finish then other engine can use, otherwise lead the error as follow sometimes:
      // D4D12 ERROR: ID3D12CommandQueue::ExecuteCommandLists: Non-simultaneous-access Texture Resource
      // (0x0000022ED17257F0:'Unnamed Object') is still referenced by write|transition_barrier GPU operations
      // in-flight on another Command Queue (0x0000022ED0F99010:'Unnamed ID3D12CommandQueue Object').
      // It is not safe to start read|write|transition_barrier GPU operations now on this Command Queue
      // (0x0000022EC7ECEAE0:'Unnamed ID3D12CommandQueue Object'). This can result in race conditions and
      // application instability. [ EXECUTION ERROR #1047: OBJECT_ACCESSED_WHILE_STILL_IN_USE]
    }
    else
      ++it;
  }
#endif
}

void ComputeEngine::generate_mipmaps() noexcept
{
  if (_mipmap_images.empty()) return;

  // TODO: need use slot, because not this mipmap can be used in current frame
  reset_cmd();

  auto cmd = Engine::cmd();

  g_desc_heap_mgr.bind_heaps(cmd);

  g_ctx.set_cmd(cmd);

  struct MipmapConstant
  {
    float2 texel_size;
    uint   mip_level{};
  };

  for (auto handle : _mipmap_images)
  {
    auto& img = g_img_mgr[handle];
    auto src_w = img.width();
    auto src_h = img.height();

    assert(!img.mipmap_descs().empty());
    auto const& src_desc = img.mipmap_descs()[0];
    img.set_state(cmd, ImageState::non_pixel, 0);

    for (auto i = 1u; i < img.mipmap_descs().size(); ++i)
    {
      // TODO: should i reset mipmap resources' state to srv?
      img.set_state(cmd, ImageState::compute_rw, i);
      g_ctx.compute_pipe_set(ComputePipeSetInfo
      {
        .type           = PipelineType::mipmap,
        .constants_name = "constants",
        .constants      = MipmapConstant
        {
          .texel_size = float2{ 1.0 / src_w, 1.0 / src_h },
          .mip_level  = i,
        },
        .descs =
        {
          { "src", src_desc.gpu_handle()              },
          { "dst", img.mipmap_descs()[i].gpu_handle() },
        }
      });

      auto dst_w = std::max(1u, img.width()  >> 1);
      auto dst_h = std::max(1u, img.height() >> 1);

      g_ctx.dispatch((dst_w + 7) / 8,  (dst_h + 7) / 8, 1);

      src_w = dst_w;
      src_h = dst_h;
    }
  }

  submit();

  // TODO: do i need to wait for graphics_engines? or set resource tracking when be used in rendering?
  // TODO: remove after use slot
  g_graphics_engine.wait(g_comp_engine);
  
  // release mipmap uavs after generation complete
  g_renderer.add_frame_render_complete_func([this, handles = std::move(_mipmap_images)]
  {
    for (auto const& handle : handles) g_img_mgr[handle].release_mipmap_descs();
  }, EngineType::compute);

  _mipmap_images.clear();
}

}
