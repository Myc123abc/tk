#include "compute_engine.hpp"
#include "../core.hpp"
#include "../resource/descriptor_heap_manager.hpp"
#include "../renderer/pipeline/pipeline_system.hpp"

#include <assert.h>

namespace {

auto boxes_for_gauss(double sigma, uint32_t n) noexcept
{
  auto w_ideal = std::sqrt((12*sigma*sigma / n) + 1);
  auto wl = static_cast<uint32_t>(std::floor(w_ideal));
  if (wl % 2 == 0) --wl;
  auto wu = wl + 2;

  auto m_ideal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n) / (-4*wl - 4);
  auto m = std::round(m_ideal);

  auto sizes = std::vector<uint32_t>(n);
  for (auto i = 0; i < n; ++i)
    sizes[i] = i < m ? wl : wu;
  return sizes;
}

}

namespace tk::renderer {

auto ComputeEngine::Slot::is_idle() const noexcept -> bool
{
  return g_comp_engine.fence_completed_value() >= fence_value;
}

ComputeEngine::Slot::Slot() noexcept
{
  cmd_alloc = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

void ComputeEngine::acquire_slot() noexcept
{
  if (auto it = std::ranges::find_if(_slots, [this](auto slot) { return slot.is_idle(); });
      it != _slots.end())
  {
    _slot = &*it;
  }
  else
  {
    _slots.emplace_back(Slot{});
    _slot = &_slots.back();
  }
  reset_cmd(_slot->cmd_alloc.Get());

  // bind heaps
  g_desc_heap_mgr.bind_heaps(cmd());
}

auto ComputeEngine::submit_slot() noexcept -> uint64_t
{
  assert(_slot && _slot->is_idle());
  _slot->fence_value = submit(); 
  return _slot->fence_value;
}

struct Constants
{
  uint32_t width{};
  uint32_t height{};
  uint32_t radius{};
  uint32_t horizontal{};
};

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

void ComputeEngine::blur(Image& src, Image& dst, float radius) noexcept
{
  acquire_slot();

  auto width  = src.width();
  auto height = src.height();

  auto [tmp_img, idx] = get_tmp_img();
  if (!tmp_img->width())
    tmp_img->init(width, height, RenderResource::Render_Target_Format, ImageType::srv | ImageType::uav);
  else
    tmp_img->resize(width, height);

  auto pipe = g_pipe_sys.pipe(PipelineType::blur);
  auto ctx  = g_pipe_sys.ctx();

  ctx->set_cmd(cmd());
  ctx->set_pipe(pipe->pipe_state.Get());
  ctx->set_compute_root_signature(pipe->root_signature);

  auto bxs = boxes_for_gauss(radius, 3);

  auto constants = Constants{ width, height };

  constants.radius     = static_cast<uint32_t>((bxs[0] - 1) / 2);
  constants.horizontal = 1;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), src.srv().gpu_handle());
  tmp_img->set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), tmp_img->uav().gpu_handle());
  ctx->dispatch(width, height);

  constants.horizontal = 0;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  tmp_img->set_state(cmd(), ImageState::common);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), tmp_img->srv().gpu_handle());
  dst.set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), dst.uav().gpu_handle());
  ctx->dispatch(width, height);

  constants.radius     = static_cast<uint32_t>((bxs[1] - 1) / 2);
  constants.horizontal = 1;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  dst.set_state(cmd(), ImageState::common);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), dst.srv().gpu_handle());
  tmp_img->set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), tmp_img->uav().gpu_handle());
  ctx->dispatch(width, height);

  constants.horizontal = 0;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  tmp_img->set_state(cmd(), ImageState::common);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), tmp_img->srv().gpu_handle());
  dst.set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), dst.uav().gpu_handle());
  ctx->dispatch(width, height);

  constants.radius     = static_cast<uint32_t>((bxs[2] - 1) / 2);
  constants.horizontal = 1;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  dst.set_state(cmd(), ImageState::common);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), dst.srv().gpu_handle());
  tmp_img->set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), tmp_img->uav().gpu_handle());
  ctx->dispatch(width, height);

  constants.horizontal = 0;
  ctx->set_compute_constants(pipe->root_param_idx("constants"), constants);
  tmp_img->set_state(cmd(), ImageState::common);
  ctx->set_compute_descriptor(pipe->root_param_idx("src"), tmp_img->srv().gpu_handle());
  dst.set_state(cmd(), ImageState::unorder_access);
  ctx->set_compute_descriptor(pipe->root_param_idx("dst"), dst.uav().gpu_handle());
  ctx->dispatch(width, height);

  _used_blur_tmp_images.emplace_back(idx, submit_slot());
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
