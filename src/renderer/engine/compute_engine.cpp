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

auto ComputeEngine::get_tmp_img() noexcept -> std::pair<Image*, uint>
{
  // refactoring code
  assert(false);
  Image* tmp_img{};
  uint   idx{};
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

void ComputeEngine::blur(Image& src, Image& dst, float sigma, uint blur_count) noexcept
{
  // refactoring code
  assert(false);

  // copy image
  // TODO: why not use copy engine
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

  auto v = _slots.submit_slot();
  _used_blur_tmp_images.emplace_back(idx, v);
  g_graphics_engine.wait(g_comp_engine, v);
}

void ComputeEngine::update() noexcept
{
  if (_mipmap_images.empty()) return;

  auto cmd = Engine::cmd();

  _slots.acquire_slot();
  g_desc_heap_mgr.bind_heaps(cmd);
  g_ctx.set_cmd(cmd);

  generate_mipmaps(cmd);

  auto fence_value = _slots.submit_slot();

  g_graphics_engine.wait(g_comp_engine, fence_value);
  
  // release mipmap descs after generation complete
  if (!_mipmap_images.empty())
  {
    g_renderer.add_frame_render_complete_func([this, handles = std::move(_mipmap_images)]
    {
      for (auto const& handle : handles) g_img_mgr[handle].release_mipmap_descs();
    }, EngineType::compute);
    _mipmap_images.clear();
  }

#if 0
  auto finish_value = fence_completed_value();
  for (auto it = _used_blur_tmp_images.begin(); it != _used_blur_tmp_images.end();)
  {
    auto [idx, fence_value] = *it;
    if (fence_value <= finish_value)
    {
      _blur_tmp_images[idx].in_use = false;
      it = _used_blur_tmp_images.erase(it);
    }
    else
      ++it;
  }
#endif
}

void ComputeEngine::generate_mipmaps(ID3D12GraphicsCommandList1* cmd) noexcept
{
  struct MipmapConstant
  {
    float2 texel_size;
  };

  for (auto handle : _mipmap_images)
  {
    auto& img   = g_img_mgr[handle];
    auto  src_w = img.width();
    auto  src_h = img.height();

    for (auto i = 0u; i < img.mipmap_descs().size(); ++i)
    {
      img.set_state(cmd, ImageState::non_pixel, i);
      img.set_state(cmd, ImageState::compute_rw, i + 1);
      auto const& [srv, uav] = img.mipmap_descs()[i];
      g_ctx.compute_pipe_set(ComputePipeSetInfo
      {
        .type           = PipelineType::mipmap,
        .constants_name = "constants",
        .constants      = MipmapConstant
        {
          .texel_size = float2{ 1.0 / src_w, 1.0 / src_h },
        },
        .descs =
        {
          { "src", srv.gpu_handle() },
          { "dst", uav.gpu_handle() },
        }
      });

      auto dst_w = std::max(1u, src_w >> 1);
      auto dst_h = std::max(1u, src_h >> 1);

      g_ctx.dispatch((dst_w + 7) / 8,  (dst_h + 7) / 8, 1);

      src_w = dst_w;
      src_h = dst_h;
    }
  }
}

}
