#pragma once

#include "../renderer/resource/image.hpp"
#include "../util/object_pool.hpp"
#include "config.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tk { namespace ui {

struct ImageInfo
{
  uint32_t width{};
  uint32_t height{};
};

class ImageManager
{
private:
  ImageManager()                           = default;
  ~ImageManager()                          = default;
public:
  ImageManager(ImageManager const&)            = delete;
  ImageManager(ImageManager&&)                 = delete;
  ImageManager& operator=(ImageManager const&) = delete;
  ImageManager& operator=(ImageManager&&)      = delete;

  static auto instance() noexcept -> ImageManager&
  {
    static ImageManager instance;
    return instance;
  }

  void destroy() noexcept;

private:
  using PoolType    = ObjectPool<ImageInfo, Image_Pool_Init_Capacity>;
public:
  using ImageHandle = PoolType::Handle;

  auto create_image(uint32_t width, uint32_t height, renderer::ImageFormat format) noexcept -> ImageHandle;
  void destroy_image(ImageHandle handle) noexcept;

  void load(std::string_view path) noexcept;
  // TODO: only call when images so much even exceed gpu memory
  void unload(std::string_view path) noexcept;

  auto contains(std::string_view path) const noexcept { return _loaded_images.contains(path.data()); }

  auto index(std::string_view path) const noexcept { return _loaded_images.at(path.data()).index(); }

private:
  PoolType _pool;
  std::unordered_map<std::string, ImageHandle>       _loaded_images;
  std::unordered_set<ImageHandle, ImageHandle::Hash> _images;
};

inline static auto& g_img_mgr{ ImageManager::instance() };

}}
