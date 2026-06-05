#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"

#include <deque>
#include <unordered_set>

namespace tk::renderer {

Singleton_Derive(ComputeEngine, g_comp_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    _slots.init(this);
  }

  void destroy() noexcept;

  void add_generate_mipmap_image(ImageHandle handle) noexcept { _mipmap_images.emplace(handle); }
  void blur(Image& src, Image& dst, float sigma, uint blur_count) noexcept;

  void update() noexcept;

private:
  void generate_mipmaps(ID3D12GraphicsCommandList1* cmd) noexcept;
  auto get_tmp_img() noexcept -> std::pair<Image*, uint>;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_COMPUTE> _slots;

  struct BlurTmpImage
  {
    ImageHandle img;
    bool        in_use{};
  };
  std::vector<BlurTmpImage>           _blur_tmp_images;
  std::deque<std::pair<uint, uint64>> _used_blur_tmp_images;
  std::unordered_set<ImageHandle>     _mipmap_images;
)

}
