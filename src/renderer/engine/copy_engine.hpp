#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"
#include "../resource/buffer.hpp"
#include "util/variant.hpp"

namespace tk::renderer {

class Command;

struct MultiBitmapCopyInfo
{
  BitmapView bitmap; 
  uint2      pos;
};

class UploadBuffer
{
public:
  void init() noexcept;
  void destroy() noexcept;

  void upload(Command* cmd) noexcept;

  void add_image(ImageHandle image, Bitmap&& bitmap) noexcept
  {
    auto& info = _infos.emplace_back();
    info.image = image;
    info.data  = std::move(bitmap);
  }

  void add_image(ImageHandle image, std::vector<MultiBitmapCopyInfo>&& infos) noexcept
  {
    auto& info = _infos.emplace_back();
    info.image = image;
    info.data  = std::move(MultiBitmapCopy{ std::move(infos) });
  }

  auto empty() const noexcept { return _infos.empty(); }

private:
  struct MultiBitmapCopy
  {
    std::vector<MultiBitmapCopyInfo> infos;
  };

  struct Info
  {
    ImageHandle                      image{};
    Variant<Bitmap, MultiBitmapCopy> data{};
  };

  BufferHandle      _buf;
  std::vector<Info> _infos;
};

Singleton_Derive(CopyEngine, g_copy_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_COPY);
    _slots.init(this);
    if (_slots.acquire_slot()) _slots.slot()->data.init();
  }

  void destroy() noexcept
  {
    Engine::destroy();
    for (auto& slot : _slots) slot.data.destroy();
  }

  void copy(Bitmap&& bitmap, ImageHandle image) noexcept
  {
    auto slot = _slots.slot();
    assert(slot && _slots.is_idle(slot));
    slot->data.upload_buf.add_image(image, std::move(bitmap));
  }

  void copy(std::vector<MultiBitmapCopyInfo>&& infos, ImageHandle img) noexcept
  {
    auto slot = _slots.slot();
    assert(slot && _slots.is_idle(slot));
    slot->data.upload_buf.add_image(img, std::move(infos));
  }

  void move(ImageHandle src, ImageHandle dst) noexcept
  {
    auto slot = _slots.slot();
    assert(slot && _slots.is_idle(slot));
    slot->data.moved_imgs.emplace_back(src, dst);
  }

  void update() noexcept;

private:
  struct MovedImage
  {
    ImageHandle src;
    ImageHandle dst;
  };
  struct SlotData
  {
    UploadBuffer            upload_buf;
    std::vector<MovedImage> moved_imgs;

    void init() noexcept { upload_buf.init(); }
    void destroy() noexcept { upload_buf.destroy(); }
  };
  Slots<D3D12_COMMAND_LIST_TYPE_COPY, SlotData> _slots;
)

}
