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

  // free render data
  if (msg.ptr)
  {
    for (auto i : std::views::iota(0, Frame_Count))
      g_render_data_pool.free(*(msg.ptr + i));
    free(msg.ptr);
  }
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
  image.init(ImageType::srv, ImageFormat::rgba8_unorm, msg.bitmap.width, msg.bitmap.height);

  // store image indexs
  renderer._image_indexs.resize(msg.index + 1);
  renderer._image_indexs.at(msg.index) = image.index();

  // store image and bitmap for upload
  renderer._upload_images[msg.index] = image;
  renderer._bitmaps[msg.index]       = msg.bitmap;
}

void Renderer::UIContextMessageHandler::operator()(Message_Remove_Image const& msg) const noexcept
{
  renderer._image_indexs.at(msg.index) = {};
  if (renderer._upload_images.contains(msg.index))
  {
    assert(renderer._bitmaps.contains(msg.index));

    // not upload yet, remove upload image
    renderer._upload_images.erase(msg.index);
    renderer._bitmaps.erase(msg.index);
  }
  else
    // already uploaded, release image resource
    renderer._images.at(msg.index).destroy();
}

}}
