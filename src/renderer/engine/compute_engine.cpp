#include "compute_engine.hpp"
#include "../resource/descriptor_heap_manager.hpp"
#include "../renderer/pipeline/pipeline_system.hpp"
#include "../resource/shader_type.hpp"
#include "graphics_engine.hpp"

#include <assert.h>
#include <ranges>

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

auto ComputeEngine::get_tmp_img() noexcept -> std::pair<Image*, uint32_t>
{
  Image*   tmp_img{};
  uint32_t idx{};
  if (auto it = std::ranges::find(_blur_tmp_images, false, &BlurTmpImage::in_use);
      it != _blur_tmp_images.end())
  {
    it->in_use = true;
    tmp_img    = &it->img;
    idx        = it - _blur_tmp_images.begin();
  }
  else
  {
    auto& res = _blur_tmp_images.emplace_back();
    res.in_use = true;
    tmp_img    = &res.img;
    idx        = _blur_tmp_images.size() - 1;
  }
  return { tmp_img, idx };
}

void ComputeEngine::blur(Image& src, Image& dst, float sigma, uint32_t blur_count) noexcept
{
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

  auto ctx             = g_pipe_sys.ctx();
  auto horizontal_pipe = g_pipe_sys.pipe(PipelineType::blur_horizontal_pass);
  auto vertical_pipe   = g_pipe_sys.pipe(PipelineType::blur_vertical_pass);

  ctx->set_cmd(cmd());
  ctx->set_compute_root_signature(horizontal_pipe->root_signature);
  ctx->set_compute_constants(horizontal_pipe->root_param_idx("constants"), constants);

  for (auto i : std::views::iota(0u, blur_count))
  {
    ctx->set_pipe(horizontal_pipe->pipe_state.Get());
    dst.set_state(cmd(), ImageState::non_pixel);
    ctx->set_compute_descriptor(horizontal_pipe->root_param_idx("src"), dst.srv().gpu_handle());
    tmp_img->set_state(cmd(), ImageState::compute_rw);
    ctx->set_compute_descriptor(horizontal_pipe->root_param_idx("dst"), tmp_img->uav().gpu_handle());
    ctx->dispatch(ceil(width / 128.f), height, 1);

    ctx->set_pipe(vertical_pipe->pipe_state.Get());
    tmp_img->set_state(cmd(), ImageState::non_pixel);
    ctx->set_compute_descriptor(vertical_pipe->root_param_idx("src"), tmp_img->srv().gpu_handle());
    dst.set_state(cmd(), ImageState::compute_rw);
    ctx->set_compute_descriptor(vertical_pipe->root_param_idx("dst"), dst.uav().gpu_handle());
    ctx->dispatch(width, ceil(height / 128.f), 1);
  }

  _used_blur_tmp_images.emplace_back(idx, _slots.submit_slot());
}

void ComputeEngine::update() noexcept
{
  auto finish_value = fence_completed_value();
  for (auto it = _used_blur_tmp_images.begin(); it != _used_blur_tmp_images.end();)
  {
    auto [idx, fence_value] = *it;
    if (fence_value <= finish_value)
    {
      _blur_tmp_images.at(idx).in_use = false;
      it = _used_blur_tmp_images.erase(it);
    }
    else
      ++it;
  }
}

}
