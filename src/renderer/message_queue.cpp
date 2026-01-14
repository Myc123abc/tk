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

}}
