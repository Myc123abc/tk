#include "renderer.hpp"
#include "engine/copy_engine.hpp"

#include <stb_image.h>

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
  image.init(ImageType::srv, ImageFormat::rgba8_unorm, msg.ptr->bitmap.width, msg.ptr->bitmap.height);
  msg.ptr->index = image.index();
  renderer._images.emplace_back(std::move(image));

  // upload data to gpu
  g_copy_engine.acquire_slot();
  g_copy_engine.copy({ msg.ptr->bitmap }, { &renderer._images.back() });
  auto _ = g_copy_engine.submit_slot();

  // free bitmap data
  stbi_image_free(msg.ptr->bitmap.data);

  // notify image create complete, can get index of descriptor now
  SetEvent(msg.ptr->event);
}

}}
