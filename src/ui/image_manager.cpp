#include "image_manager.hpp"
#include "util/error_handling.hpp"
#include "../renderer/renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace tk::renderer;

namespace tk { namespace ui {

void ImageManager::load(std::string_view path) noexcept
{
  assert(!_slot_indexs.contains(path.data()));

  // load bitmap
  int  w, h, ch;
  auto data = stbi_load(path.data(), &w, &h, &ch, 4);
  err_if(!data, "not found image {}", path);

  // check whether have free slot
  if (!_free_slots.empty())
  {
    // pop free slot
    auto idx = _free_slots.front();
    _slot_indexs.emplace(path, idx);
    _free_slots.pop();

    // set image info
    auto& slot = _slots.at(idx);
    slot.info.width   = w;
    slot.info.height  = h;
    slot.info.channel = ch;

     // send message to renderer
    auto msg = Renderer::Message_Upload_Image{};
    msg.bitmap.init(w, h, 4, data);
    msg.index = idx;
    g_renderer.send_message(std::move(msg));
    return;
  }

  // send message to renderer
  auto msg = Renderer::Message_Upload_Image{};
  msg.bitmap.init(w, h, 4, data);
  msg.index = _slots.size();
  g_renderer.send_message(std::move(msg));

  // create slot
  auto slot = Slot{};
  slot.info.index   = _slots.size();
  slot.info.width   = w;
  slot.info.height  = h;
  slot.info.channel = ch;
  _slot_indexs.emplace(path, slot.info.index);
  _slots.emplace_back(std::move(slot));
}

void ImageManager::unload(std::string_view path) noexcept
{
  assert(_slot_indexs.contains(path.data()));
  auto idx = _slot_indexs.at(path.data());
  _free_slots.push(idx);
  _slot_indexs.erase(path.data());

  g_renderer.send_message(Renderer::Message_Remove_Image{ idx });
}

}}
