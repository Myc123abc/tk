#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "image_manager.hpp"
#include "../renderer/renderer/renderer.hpp"
#include "util/file.hpp"

#include <ranges>

using namespace tk::renderer;

namespace tk::ui {

void ImageManager::destroy() noexcept
{
  for (auto handle : _loaded_images | std::views::values) _pool.free(handle);
  for (auto handle : _images) _pool.free(handle);
}

auto ImageManager::create_image(uint32_t width, uint32_t height, ImageFormat format) noexcept -> ImageHandle
{
  auto handle = _pool.alloc();
  _images.emplace(handle);
  auto& info = _pool[handle];
  info.width  = width;
  info.height = height;
  g_renderer.create_image(handle, width, height, format);
  return handle;
}

void ImageManager::destroy_image(ImageHandle handle) noexcept
{
  assert(_images.contains(handle));

  // remove image resource by index
  g_renderer.destroy_image(handle);

  // remove image
  _images.erase(handle);

  // release image handle
  _pool.free(handle);
}

auto ImageManager::extent(std::string_view path) noexcept -> glm::vec2
{
  if (!_image_extents.contains(path.data()))
  {
    int w{}, h{};
    stbi_info(path.data(), &w, &h, nullptr);
    _image_extents[path.data()] = { w, h };
  }
  return _image_extents[path.data()];
}

auto ImageManager::try_load(std::string_view path, glm::vec2 extent) noexcept -> bool
{
  if (_load_tasks.contains(path.data()))
  {
    if (_load_tasks[path.data()].is_completed())
    {
      debug("fine {}", path);
      auto task = std::move(_load_tasks[path.data()]);
      _load_tasks.erase(path.data());
      auto [data, w, h] = task.take_result();
      if (!data) return false;
      load(path, w, h, data);
      return true;
    }
    return false;
  }
  if (!_loaded_images.contains(path.data()))
  {
    debug("load start {}", path);
    auto beg = std::chrono::steady_clock::now();
    _load_tasks.emplace(path, g_thread_pool.submit([path = std::string(path)]
    {
      debug("parse {}", path);
      int w, h; 
      auto file = File{ path };
      auto data = stbi_load_from_memory(file.data<stbi_uc>(), file.size(), reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), nullptr, 4);
      if (!data) data = nullptr;
      return LoadResult{ data, w, h };
    }));
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
    debug("load end {}\ntime {}", path, dur);
    return false;
  }
  return true;
}

void ImageManager::load(std::string_view path, uint32_t width, uint32_t height, void* data, bool use_mipmap) noexcept
{
  assert(!_loaded_images.contains(path.data()));

  auto handle = _pool.alloc();
  _loaded_images.emplace(path, handle);
  _pool[handle] = { width, height, use_mipmap };

  if (!_image_extents.contains(path.data()))
    _image_extents[path.data()] = { width, height };

  auto bitmap = Bitmap{};
  bitmap.init(width, height, 4, data);
  g_renderer.upload_image(handle, width, height, bitmap, use_mipmap);
}

void ImageManager::unload(std::string_view path) noexcept
{
  assert(_loaded_images.contains(path.data()));

  // get image handle
  auto handle = _loaded_images[path.data()];

  // remove image resource by index
  g_renderer.destroy_image(handle);

  // remove image record
  _loaded_images.erase(path.data());

  // release image handle
  _pool.free(handle);
}

}
