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
  }

  auto& operator[](ImageHandle handle) noexcept { return _pool[handle]; }
  auto get(ImageHandle handle) noexcept { return _pool.get(handle); }

  auto extent(std::string_view path) noexcept -> float2;
  auto try_load(std::string_view path, uint w, uint h) noexcept -> std::expected<ImageHandle, ui::ImageLoadError::Type>;
  void update() noexcept;

  auto blur(ImageHandle handle, float2 ext, uint radius, float sigma) noexcept -> ImageHandle;

private:
  void load(std::string_view path, uint width, uint height, void* data, bool use_mipmap) noexcept;
  auto find_tmp_image(uint width, uint height, ImageFormat fmt, Flag<ImageType> types) noexcept -> uint;

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

  struct TmpImage
  {
    ImageHandle img;
    bool        is_used{};
  };
  std::vector<TmpImage> _tmp_imgs;
  struct BlurImage
  {
    ImageHandle img;
    size_t      hash{};
  };
  std::unordered_map<ImageHandle, BlurImage> _blur_imgs;
)

}

namespace tk::ui {

using ImageHandle = renderer::ImageHandle;

}
