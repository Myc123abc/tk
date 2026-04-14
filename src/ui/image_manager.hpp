#pragma once

#include "../renderer/resource/image.hpp"
#include "../util/object_pool.hpp"
#include "config.hpp"
#include "../util/singleton.hpp"
#include "../util/thread_pool.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tk::ui {

Singleton(ImageManager, g_img_mgr,
public:
  void destroy() noexcept;

private:
  struct ImageInfo
  {
    uint32_t width{};
    uint32_t height{};
    bool     has_mipmap{};

    auto extent() const noexcept -> glm::vec2 { return { width, height }; }

    ImageInfo() noexcept = default;
    ImageInfo(uint32_t width, uint32_t height, bool has_mipmap) noexcept
      : width(width), height(height), has_mipmap(has_mipmap) {}
  };

  using PoolType    = ObjectPool<ImageInfo, Image_Pool_Init_Capacity>;
public:
  using ImageHandle = PoolType::Handle;

  auto create_image(uint32_t width, uint32_t height, renderer::ImageFormat format) noexcept -> ImageHandle;
  void destroy_image(ImageHandle handle) noexcept;

  auto try_load(std::string_view path, glm::vec2 extent) noexcept -> bool;
  void unload(std::string_view path) noexcept;

  void try_generate_mipmap(glm::vec2 extent) const noexcept;

  auto contains(std::string_view path) const noexcept { return _loaded_images.contains(path.data());   }
  auto extent(std::string_view path) noexcept -> glm::vec2;
  auto handle(std::string_view path) noexcept { return _loaded_images[path.data()]; }

private:
  void load(std::string_view path, uint32_t width, uint32_t height, void* data, bool use_mipmap = false) noexcept;

private:
  PoolType                                     _pool;
  std::unordered_map<std::string, ImageHandle> _loaded_images;
  std::unordered_set<ImageHandle>              _images;
  std::unordered_map<std::string, glm::vec2>   _image_extents;

  struct LoadResult
  {
    void* data{};
    int   w{}, h{};
  };
  std::unordered_map<std::string, Task<LoadResult>> _load_tasks;
)

using ImageHandle = ImageManager::ImageHandle;

}
