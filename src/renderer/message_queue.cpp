#include "renderer.hpp"

namespace tk { namespace renderer {

void Renderer::MessageHandler::operator()(Message_Window_Create const& msg) const noexcept
{
  err_if(renderer._res.contains(msg.handle), "failed to create window render resource, it's already exist");
  auto res = RenderResource{};
  res.init(msg.handle, msg.width, msg.height);
  renderer._res.emplace(msg.handle, std::move(res));
}

void Renderer::MessageHandler::operator()(Message_Window_Destroy const& msg) const noexcept
{
  err_if(!renderer._res.contains(msg.handle), "failed to destroy window render resource, it's unexist");
  renderer.add_frame_render_complete_func([handle = msg.handle, res = std::move(renderer._res.at(msg.handle))] mutable
  {
    res.destroy();
    DestroyWindow(handle);
  });
  renderer._res.erase(msg.handle);
  renderer._destroied_windows.emplace(msg.handle);
}

void Renderer::MessageHandler::operator()(Message_Window_Update const& msg) const noexcept
{
  err_if(!renderer._res.contains(msg.handle), "failed to destroy window render resource, it's unexist");
  renderer._res[msg.handle].resize(msg.width, msg.height);
}

void Renderer::UIContextMessageHandler::operator()(Message_Upload_Image const& msg) const noexcept
{
  // create image resource
  auto image = Image{};
  image.init(ImageType::srv, ImageFormat::rgba8_unorm, msg.bitmap.width, msg.bitmap.height, msg.use_mipmap);
  if (msg.use_mipmap)
    renderer._pending_mipmap_indices.emplace_back(msg.index);

  // store image indices
  renderer._image_indices.resize(msg.index + 1);
  renderer._image_indices.at(msg.index) = image.index();

  // store image and bitmap for upload
  renderer._upload_images[msg.index] = image;
  renderer._bitmaps[msg.index]       = msg.bitmap;
}

void Renderer::UIContextMessageHandler::operator()(Message_Create_Image const& msg) const noexcept
{
  // create image resource
  auto image = Image{};
  image.init(ImageType::srv, msg.format, msg.width, msg.height);

  // store image indices
  renderer._image_indices.resize(msg.index + 1);
  renderer._image_indices.at(msg.index) = image.index();

  assert(!renderer._images.contains(msg.index));
  renderer._images.emplace(msg.index, std::move(image));
}

void Renderer::UIContextMessageHandler::operator()(Message_Destroy_Image const& msg) const noexcept
{
  renderer._image_indices.at(msg.index) = {};
  if (renderer._upload_images.contains(msg.index))
  {
    assert(renderer._bitmaps.contains(msg.index));

    // not upload yet, remove upload image
    renderer._upload_images.erase(msg.index);
    renderer._bitmaps.erase(msg.index);
  }
  else
  {
    // already uploaded, release image resource
    renderer.add_frame_render_complete_func([image = renderer._images.at(msg.index)] mutable { image.destroy(); });
    renderer._images.erase(msg.index);
  }
}

}}
