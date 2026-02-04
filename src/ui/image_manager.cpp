#include "image_manager.hpp"
#include "util/error_handling.hpp"
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

void ImageManager::load(std::string_view path) noexcept
{
  assert(!_loaded_images.contains(path.data()));

  auto handle = _pool.alloc();
  _loaded_images.emplace(path, handle);

  auto& info = _pool[handle];

  // load bitmap
  auto data = stbi_load(path.data(), reinterpret_cast<int*>(&info.width), reinterpret_cast<int*>(&info.height), nullptr, 4);
  err_if(!data, "not found image {}", path);

   // send message to renderer
  auto msg = Renderer::Message_Upload_Image{};
  msg.bitmap.init(info.width, info.height, 4, data);
  msg.index = handle.index();
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

}}
