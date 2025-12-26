#include "renderer.hpp"

namespace tk { namespace renderer {

void Renderer::MessageHandler::operator()(Message_Window_Create msg) const noexcept
{
  err_if(renderer._res.contains(msg.handle), "failed to create window render resource, it's already exist");
  auto res = RenderResource{};
  res.init(msg.handle, msg.width, msg.height);
  renderer._res.emplace(msg.handle, std::move(res));
}

void Renderer::MessageHandler::operator()(Message_Window_Destroy msg) const noexcept
{
  err_if(!renderer._res.contains(msg.handle), "failed to destroy window render resource, it's unexist");
  renderer.add_frame_render_complete_func([handle = msg.handle, res = std::move(renderer._res.at(msg.handle))] mutable
  {
    res.destroy();
    DestroyWindow(handle);
  });
  renderer._res.erase(msg.handle);
}

}}
