#include "renderer.hpp"
#include "util/error_handling.hpp"

namespace tk::renderer {

void Renderer::MessageHandler::operator()(Message_Window_Create const& msg) const noexcept
{
  err_if(renderer._res.contains(msg.handle), "failed to create window render resource, it's already exist");
  auto res = RenderResource{};
  res.init(msg.handle, msg.width, msg.height);
  renderer._res.emplace(msg.handle, std::move(res));
  renderer._render_datas.emplace(msg.handle, RenderData{});
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
  renderer._render_datas.erase(msg.handle);
  renderer._destroied_windows.emplace(msg.handle);
}

void Renderer::MessageHandler::operator()(Message_Window_Update const& msg) const noexcept
{
  err_if(!renderer._res.contains(msg.handle), "failed to destroy window render resource, it's unexist");
  renderer._res[msg.handle].resize(msg.width, msg.height);
}

void Renderer::MessageHandler::operator()(Message_Show_Blur_Window const& msg) const noexcept
{
  renderer._show_blur_wnds.emplace(msg.handle, msg.blur_handle);
}

void Renderer::UIContextMessageHandler::operator()(Message_Upload_Image const& msg) const noexcept
{
  // create image resource
  auto image = Image{};
  image.init(msg.bitmap.width, msg.bitmap.height, ImageFormat::rgba8_unorm, ImageType::srv, msg.use_mipmap);

  // store image and bitmap for upload
  renderer._upload_images[msg.handle] = std::move(image);
  renderer._bitmaps[msg.handle]       = msg.bitmap;

  if (msg.use_mipmap)
    renderer._pending_mipmap_image_handles.emplace_back(msg.handle);
}

void Renderer::UIContextMessageHandler::operator()(Message_Create_Image const& msg) const noexcept
{
  // create image resource
  auto image = Image{};
  image.init(msg.width, msg.height, msg.format, ImageType::srv);

  assert(!renderer._images.contains(msg.handle));
  renderer._images.emplace(msg.handle, std::move(image));
}

void Renderer::UIContextMessageHandler::operator()(Message_Destroy_Image const& msg) const noexcept
{
  if (renderer._upload_images.contains(msg.handle))
  {
    assert(renderer._bitmaps.contains(msg.handle));

    // not upload yet, remove upload image
    renderer._upload_images.erase(msg.handle);
    renderer._bitmaps.erase(msg.handle);
  }
  else if (renderer._images.contains(msg.handle))
  {
    // already uploaded, release image resource
    renderer.add_frame_render_complete_func([image = std::move(renderer._images.at(msg.handle))] mutable { image.destroy(); });
    renderer._images.erase(msg.handle);
  }
}

}
