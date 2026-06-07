#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "image_manager.hpp"
#include "../../util/file_manager.hpp"
#include "../../util/hash.hpp"
#include "../engine/copy_engine.hpp"
#include "../engine/compute_engine.hpp"

#include <algorithm>

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
  if (!_image_infos.contains(path.data()))
  {
    int w, h;
    stbi_info(path.data(), &w, &h, nullptr);
    _image_infos[path.data()] = { { w, h } };
  }
  return _image_infos[path.data()].extent;
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

  if (_loaded_images.contains(filename))
  {
    // generate mipmap if render size is too small
    assert(_image_infos.contains(filename));
    auto& info = _image_infos[filename];
    if (!info.use_mipmap)
    {
      auto ratio_x = static_cast<float>(width)  / info.extent.x;
      auto ratio_y = static_cast<float>(height) / info.extent.y;
      if (ratio_x > 0 && ratio_y > 0 && (ratio_x <= 0.5 || ratio_y <= 0.5))
      {
        info.use_mipmap = true;
        
        // get current image handle
        auto old_handle = _loaded_images[filename];
        
        // recreate an image with mipmap
        auto const& old_img = _pool[old_handle];
        auto new_handle = create(old_img.width(), old_img.height(), old_img.format(), old_img.types(), true);
        _loaded_images[filename] = new_handle;

        // for new image with mipmap, we need to copy the old image's content to new image
        g_copy_engine.move(old_handle, new_handle);
        // and generate mipmaps by compute engine
        g_comp_engine.add_generate_mipmap_image(new_handle);
      }
    }
    return _loaded_images[filename];
  }

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

  if (!_image_infos.contains(path.data()))
    _image_infos[path.data()] = { { width, height }, use_mipmap };

  g_copy_engine.copy({ width, height, 4, data, true }, handle);
}

/*
TODO:
the blur image generate need to be async avoid block the main loop if gpu need spent too many time.
and avoid the frame result not consistency like some controls are changed but image is still not blured,
we need to build the state for per frame to verify whether all commands can run.
*/
auto ImageManager::blur(ImageHandle handle, float2 ext, float sigma, uint cnt) noexcept -> ImageHandle
{
  auto const& src = g_img_mgr[handle];
  ext = min(ext, src.extent());

  auto hash = generic_hash(ext.x, ext.y, sigma, cnt);

  // find whether this image has blured image
  if (!_blur_imgs.contains(handle))
  {
    // when not have blur image, create a blur image
    auto fmt   = src.format();
    auto types = ImageType::srv | ImageType::uav;

    auto& blur_img = _blur_imgs[handle];
    blur_img.img  = create(ext.x, ext.y, fmt, types);
    blur_img.hash = hash;

    // find a temp image for blur image generation
    auto tmp_img = find_tmp_image(ext.x, ext.y, fmt, types);
    // TODO: when image not blur again, remeber remove the blur image resource

    // TODO: submit to compute engine for blur generatation
    g_comp_engine.blur(handle, blur_img.img, tmp_img, ext, sigma, cnt);

    return blur_img.img;
  }

  // when this image have the blur image, check whether the image size and blur setting are changed
  auto& blur_img = _blur_imgs[handle];
  if (blur_img.hash == hash)
  {
    // when is the same setting, so nothing need to do, the blur image should also be cached, directly use it
    return blur_img.img;
  }

  // the blur setting is changed, need to regenerate the blur image
  blur_img.hash = hash;
  // TODO: delete old blur image and create new one
  // TODO: regenerate blur image and submit to compute engine
  return {};
}

auto ImageManager::find_tmp_image(uint width, uint height, ImageFormat fmt, Flag<ImageType> types) noexcept -> ImageHandle
{
  // have idle tmp images, find a suitable one
  if (auto it = std::ranges::find_if(_tmp_imgs, [&](auto const& pair)
      {
        if (pair.second) return false;
        auto const& tmp_img = g_img_mgr[pair.first];
        return tmp_img.width() >= width && tmp_img.height() >= height && tmp_img.format() == fmt && tmp_img.types() == types;
      }); it != _tmp_imgs.end())
  {
    // find sutiable tmp image, use it
    it->second = true;
    return it->first;
  }

  // reuse any idle tmp image by recreating it with the requested format/type
  if (auto it = std::ranges::find_if(_tmp_imgs, [&](auto const& pair) { return !pair.second; });
      it != _tmp_imgs.end())
  {
    it->second = true;
    g_img_mgr[it->first].init(width, height, fmt, types);
    return it->first;
  }

  // otherwise, create a new one
  auto handle = create(width, height, fmt, types);
  _tmp_imgs.emplace(handle, true);
  return handle;
}

void ImageManager::tmp_img_used_finish(ImageHandle handle) noexcept
{
  assert(_tmp_imgs.contains(handle));
  auto& is_used = _tmp_imgs[handle];
  assert(is_used);
  is_used = false;
}

}
