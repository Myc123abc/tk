#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "image_manager.hpp"
#include "../../util/file_manager.hpp"
#include "../engine/copy_engine.hpp"
#include "../engine/compute_engine.hpp"

using namespace tk::ui;

namespace tk::renderer {

auto ImageManager::create(uint width , uint height, ImageFormat format, Flag<ImageType> types, bool use_mipmap) noexcept -> ImageHandle
{
  auto handle = _pool.alloc();
  _pool[handle].init(width, height, format, types, use_mipmap);
  if (use_mipmap) g_comp_engine.add_generate_mipmap_image(handle);
  return handle;
}

auto ImageManager::extent(std::string_view path) noexcept -> float2
{
  if (!_image_extents.contains(path.data()))
  {
    int w, h;
    stbi_info(path.data(), &w, &h, nullptr);
    _image_extents[path.data()] = { w, h };
  }
  return _image_extents[path.data()];
}

auto ImageManager::try_load(std::string_view path, uint width, uint height) noexcept -> std::expected<ImageHandle, ImageLoadError::Type>
{
  auto filename = path.data();

  if (_decoded_failed_images.contains(filename))
  {
    if (g_file_mgr.is_updated(filename))
      _decoded_failed_images.erase(filename);
    else
      return std::unexpected(ImageLoadError::decode_failed{ _decoded_failed_images[filename] });
  }

  if (_loaded_images.contains(filename)) return _loaded_images[filename];

  if (!g_file_mgr.exists(path)) return std::unexpected(ImageLoadError::unexist{});

  // load image if not loaded
  if (!_load_tasks.contains(filename))
  {
    auto task = g_thread_pool.submit([path = std::string(path), width, height]
    {
      int w, h; 
      std::string msg;
      auto& file = g_file_mgr[g_file_mgr.load(path)];
      auto  data = stbi_load_from_memory(file.data<stbi_uc>(), file.size(), reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), nullptr, 4);
      if (!data) msg = stbi_failure_reason();

      auto ratio_x    = static_cast<float>(width) / w;
      auto ratio_y    = static_cast<float>(height) / h;
      auto use_mipmap = false;
      if (ratio_x > 0 && ratio_y > 0) use_mipmap = ratio_x <= 0.5 || ratio_y <= 0.5;
      return LoadResult{ data, w, h, msg, use_mipmap };
    });
    _load_tasks.emplace(path, std::move(task));
  }
  return std::unexpected(ImageLoadError::loading{});
}

void ImageManager::update() noexcept
{
  // check images whether loeaded
  for (auto it = _load_tasks.begin(); it != _load_tasks.end();)
  {
    auto const& path = it->first;
    if (_load_tasks[path.data()].is_completed())
    {
      auto path_cpy = path;
      auto task = std::move(_load_tasks[path_cpy]);
      it = _load_tasks.erase(it);
      auto [data, w, h, msg, use_mipmap] = task.take_result();
      if (!data)
      {
        auto res = _decoded_failed_images.emplace(path_cpy, msg);
        assert(res.second);
        continue;
      }
      load(path_cpy, w, h, data, use_mipmap);
    }
    else ++it;
  }
}

void ImageManager::load(std::string_view path, uint width, uint height, void* data, bool use_mipmap) noexcept
{
  assert(!_loaded_images.contains(path.data()));

  auto handle = create(width, height, ImageFormat::rgba8_unorm, ImageType::srv, use_mipmap);
  _loaded_images.emplace(path, handle);

  if (!_image_extents.contains(path.data()))
    _image_extents[path.data()] = { width, height };

  g_copy_engine.copy({ width, height, 4, data, true }, handle);
}

}
