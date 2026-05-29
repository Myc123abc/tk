#pragma once

#include "slots.hpp"
#include "../resource/image_manager.hpp"
#include "../resource/buffer.hpp"
#include "util/variant.hpp"

namespace tk::renderer {

struct MultiBitmapCopyInfo
{
  BitmapView bitmap; 
  uint2      pos;
};

class UploadBuffer
{
public:
  void upload(ID3D12GraphicsCommandList1* cmd) noexcept;

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

  Buffer            _buffer;
  std::vector<Info> _infos;
};

Singleton_Derive(CopyEngine, g_copy_engine, Engine,
public:
  void init() noexcept
  {
    Engine::init(D3D12_COMMAND_LIST_TYPE_COPY);
    _slots.init(this);
    _slots.acquire_slot();
  }

  void copy(Bitmap&& bitmap, ImageHandle image) noexcept
  {
    auto slot = _slots.slot();
    assert(slot && _slots.is_idle(slot));
    slot->data.add_image(image, std::move(bitmap));
  }

  void copy(std::vector<MultiBitmapCopyInfo>&& infos, ImageHandle img) noexcept
  {
    auto slot = _slots.slot();
    assert(slot && _slots.is_idle(slot));
    slot->data.add_image(img, std::move(infos));
  }

  void update() noexcept;

private:
  Slots<D3D12_COMMAND_LIST_TYPE_COPY, UploadBuffer> _slots;
)

}
