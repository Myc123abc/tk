#include "image_manager.hpp"
#include "../renderer/renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace tk::renderer;

namespace tk { namespace ui {

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
  g_renderer.send_message(Renderer::Message_Create_Image{ width, height, format, handle.index() });
  return handle;
}

void ImageManager::destroy_image(ImageHandle handle) noexcept
{
  assert(_images.contains(handle));

  // remove image resource by index
  g_renderer.send_message(Renderer::Message_Destroy_Image{ handle.index() });

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
  return _image_extents.at(path.data());
}

auto ImageManager::try_load(std::string_view path, glm::vec2 extent) noexcept -> bool
{
  if (_loaded_images.contains(path.data()))
  {
    auto const& info = _pool[_loaded_images.at(path.data())];
    if (!info.has_mipmap && (extent.x < info.width || extent.y < info.height))
      generate_mipmap(path);
  }
  else
  {
    int w, h; 
    auto data = stbi_load(path.data(), reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), nullptr, 4);
    if (!data) return false;
    load(path, w, h, data, extent.x < w || extent.y < h);
  }
  return true;
}

void ImageManager::load(std::string_view path, uint32_t width, uint32_t height, void* data, bool use_mipmap) noexcept
{
  assert(!_loaded_images.contains(path.data()));

  auto handle = _pool.alloc();
  _loaded_images.emplace(path, handle);
  _pool[handle] = { width, height, use_mipmap };

  // send message to renderer
  auto msg = Renderer::Message_Upload_Image{};
  msg.bitmap.init(width, height, 4, data);
  msg.index      = handle.index();
  msg.use_mipmap = use_mipmap;
  g_renderer.send_message(std::move(msg));
}

void ImageManager::unload(std::string_view path) noexcept
{
  assert(_loaded_images.contains(path.data()));

  // get image handle
  auto handle = _loaded_images.at(path.data());

  // remove image resource by index
  g_renderer.send_message(Renderer::Message_Destroy_Image{ handle.index() });

  // remove image record
  _loaded_images.erase(path.data());

  // release image handle
  _pool.free(handle);
}

void ImageManager::generate_mipmap(std::string_view path) noexcept
{
  assert(_loaded_images.contains(path.data()));
  auto& info = _pool[_loaded_images.at(path.data())];
  info.has_mipmap = true;
  // TODO:
  // copy srv image to uav image
  // use uav image's mipmap uavs to generate mipmaps
  // and destroy old resources with fence complete
}

}}
