#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"

#include <deque>
#include <vector>

namespace tk::renderer {

Singleton_Derive(ComputeEngine, g_comp_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    _slots.init(this);
  }

  void destroy() noexcept;

  void blur(Image& src, Image& dst, float sigma, uint32_t blur_count) noexcept;

  void update() noexcept;

private:
  auto get_tmp_img() noexcept -> std::pair<Image*, uint32_t>;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_COMPUTE> _slots;

  struct BlurTmpImage
  {
    ImageHandle img;
    bool        in_use{};
  };
  std::vector<BlurTmpImage>                 _blur_tmp_images;
  std::deque<std::pair<uint32_t, uint64_t>> _used_blur_tmp_images;
)

}
