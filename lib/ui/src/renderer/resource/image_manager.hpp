#pragma once

#include "image.hpp"
#include "../../util/object_pool.hpp"
#include "../../util/thread_pool.hpp"
#include "../config.hpp"
#include "ui/ui.hpp"

#include <ranges>

namespace tk::renderer {

using ImagePoolType = ObjectPool<Image, Image_Pool_Init_Capacity>;
using ImageHandle   = ImagePoolType::Handle;

Singleton(ImageManager, g_img_mgr,
public:
  auto create(uint width , uint height, ImageFormat format, Flag<ImageType> types, bool use_mipmap = false) noexcept -> ImageHandle;

  auto create(IDXGISwapChain1* swapchain, uint index) noexcept
  {
    auto handle = _pool.alloc();
    _pool[handle].init(swapchain, index);
    return handle;
  }

  auto create(float width, float height, Image const& src) noexcept
  {
    auto handle = _pool.alloc();
    _pool[handle].init(width, height, src);
    return handle;
  }

  auto destroy(ImageHandle handle) noexcept
  {
    _pool[handle].destroy();
    _pool.free(handle);
  }

  auto destroy() noexcept
  {
    for (auto handle : _loaded_images | std::views::values) destroy(handle);
    for (auto handle : _tmp_imgs | std::views::keys) destroy(handle);
    for (auto const& img : _blur_imgs | std::views::values) destroy(img.image);
    for (auto img : _destroy_blur_imgs) destroy(img);
  }

  auto& operator[](ImageHandle handle) noexcept { return _pool[handle]; }
  auto get(ImageHandle handle) noexcept { return _pool.get(handle); }

  auto extent(std::string_view path) noexcept -> float2;
  auto try_load(std::string_view path, uint w, uint h) noexcept -> std::expected<ImageHandle, ui::ImageLoadErrorType>;
  void update() noexcept;

  auto blur(ImageHandle handle, float2 ext, float sigma, uint cnt) noexcept -> ImageHandle;
  void tmp_img_used_finish(ImageHandle handle) noexcept;

private:
  void load(std::string_view path, uint width, uint height, void* data, bool use_mipmap) noexcept;
  auto find_tmp_image(uint width, uint height, ImageFormat fmt, Flag<ImageType> types) noexcept -> ImageHandle;

private:
  ImagePoolType _pool;

  struct LoadResult
  {
    void*       data{};
    int         w{}, h{};
    std::string err_msg;
    bool        use_mipmap{};
  };
  struct ImageInfo
  {
    uint2 extent;
    bool  use_mipmap{};
  };
  std::unordered_map<std::string, Task<LoadResult>> _load_tasks;
  std::unordered_map<std::string, ImageHandle>      _loaded_images;
  std::unordered_map<std::string, ImageInfo>        _image_infos;
  std::unordered_map<std::string, std::string>      _decoded_failed_images;

  struct BlurImage
  {
    ImageHandle image;
    uint2       extent;
    float       sigma{};
    uint        cnt{};
    bool        used{};
  };

  std::unordered_map<ImageHandle, bool>      _tmp_imgs;
  std::unordered_map<ImageHandle, BlurImage> _blur_imgs;
  std::vector<ImageHandle>                   _destroy_blur_imgs;
)

}

namespace tk::ui {

using ImageHandle = renderer::ImageHandle;

}
