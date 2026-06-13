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
  Engine::destroy();
}

void ComputeEngine::update() noexcept
{
  if (_mipmap_images.empty() && _blur_imgs.empty()) return;

  auto cmd = Engine::cmd();

  _slots.acquire_slot();
  cmd->bind_descriptor_heaps();
  g_ctx.set_cmd(cmd);

  generate_mipmaps(cmd);
  blur(cmd);

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

  // mark tmp images are used finish after blur process is completely
  if (!_blur_imgs.empty())
  {
    auto tmp_imgs = _blur_imgs
      | std::views::transform([](auto const& b) { return b.tmp; })
      | std::ranges::to<std::vector<ImageHandle>>();
    g_renderer.add_frame_render_complete_func([tmp_imgs = std::move(tmp_imgs)]
    {
      for (auto h : tmp_imgs) g_img_mgr.tmp_img_used_finish(h);
    // TODO: make pre frame process can use gave fence values
    }, EngineType::compute);
    _blur_imgs.clear();
  }
}

void ComputeEngine::generate_mipmaps(Command const* cmd) noexcept
{
  if (_mipmap_images.empty()) return;

  struct MipmapConstant
  {
    float2 texel_size;
  };

  for (auto handle : _mipmap_images)
  {
    auto& img   = g_img_mgr[handle];
    auto  src_w = img.width();
    auto  src_h = img.height();

    auto i = 0;
    for (; i < img.mipmap_descs().size(); ++i)
    {
      // TODO: batch barriers
      cmd->transform(handle, ImageState::non_pixel, i);
      cmd->transform(handle, ImageState::compute_rw, i + 1);
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
    cmd->transform(handle, ImageState::non_pixel, i);
  }
}

void ComputeEngine::blur(Command const* cmd) noexcept
{
  if (_blur_imgs.empty()) return;

  auto const& horizontal_pipe = g_pipe_sys.pipe(PipelineType::blur_horizontal_pass);
  auto const& vertical_pipe   = g_pipe_sys.pipe(PipelineType::blur_vertical_pass);

  auto constants = BlurConstants{};

  for (auto const& [src_h, dst_h, tmp_h, ext, sigma, cnt] : _blur_imgs)
  {
    auto& src = g_img_mgr[src_h];
    auto& dst = g_img_mgr[dst_h];
    auto& tmp = g_img_mgr[tmp_h];

    assert(ext.x <= dst.width() && ext.y <= dst.height() &&
           ext.x <= tmp.width() && ext.y <= tmp.height());

    // calculate widgets and blur radius
    if (!_weights.contains(sigma))
      _weights.emplace(sigma, get_gauss_weights(sigma));
    auto const& widgets = _weights[sigma];
    constants.blur_radius = widgets.size() / 2;
    memcpy(&constants.widgets, widgets.data(), widgets.size() * sizeof(float));

    g_ctx.set_compute_root_signature(horizontal_pipe->root_signature);
    g_ctx.set_compute_constants(horizontal_pipe->root_param_idx("constants"), constants);

    for (auto i = 0; i < cnt; ++i)
    {
      cmd->transform(dst_h, ImageState::non_pixel);
      cmd->transform(tmp_h, ImageState::compute_rw);

      g_ctx.set_pipe(horizontal_pipe->pipe_state.Get());
      g_ctx.set_compute_descriptor(horizontal_pipe->root_param_idx("src"), dst.srv().gpu_handle());
      g_ctx.set_compute_descriptor(horizontal_pipe->root_param_idx("dst"), tmp.uav().gpu_handle());
      g_ctx.dispatch(ceil(ext.x / 128.f), ext.y, 1);

      cmd->transform(tmp_h, ImageState::non_pixel);
      cmd->transform(dst_h, ImageState::compute_rw);

      g_ctx.set_pipe(vertical_pipe->pipe_state.Get());
      g_ctx.set_compute_descriptor(vertical_pipe->root_param_idx("src"), tmp.srv().gpu_handle());
      g_ctx.set_compute_descriptor(vertical_pipe->root_param_idx("dst"), dst.uav().gpu_handle());
      g_ctx.dispatch(ext.x, ceil(ext.y / 128.f), 1);
    }
  }
}

}
