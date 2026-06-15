#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"

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

  void add_generate_mipmap_image(ImageHandle handle) noexcept
  {
    _mipmap_images.emplace(handle);
    g_img_mgr[handle].compute_will_use();
  }

  void blur(ImageHandle src, ImageHandle dst, ImageHandle tmp, float2 ext, float sigma, uint cnt) noexcept
  {
    _blur_imgs.emplace_back(src, dst, tmp, ext, sigma, cnt);
    g_img_mgr[src].compute_will_use();
    g_img_mgr[dst].compute_will_use();
    g_img_mgr[tmp].compute_will_use();
  }

  void update() noexcept;

private:
  void generate_mipmaps(Command const* cmd) noexcept;
  void blur(Command const* cmd) noexcept;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_COMPUTE> _slots;
  std::unordered_set<ImageHandle>        _mipmap_images;

  struct BlurImage
  {
    ImageHandle src;
    ImageHandle dst;
    ImageHandle tmp;
    float2      ext;
    float       sigma{};
    uint        cnt{};
  };
  std::vector<BlurImage>                        _blur_imgs;
  std::unordered_map<float, std::vector<float>> _weights;
)

}
