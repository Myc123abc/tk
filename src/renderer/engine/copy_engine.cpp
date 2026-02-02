#include "copy_engine.hpp"
#include "../../util/align.hpp"
#include "../core.hpp"

namespace tk { namespace renderer {

////////////////////////////////////////////////////////////////////////////////
///                             Upload Buffer
////////////////////////////////////////////////////////////////////////////////

void UploadBuffer::add_images(std::vector<Image*> const& images, std::vector<Bitmap> const& bitmaps) noexcept
{
  assert(images.size() == bitmaps.size());

  _infos.reserve(_infos.size() + images.size());
  for (auto i = 0; i < images.size(); ++i)
  {
    auto info = Info{};
    info.image           = images[i];
    info.data.pData      = bitmaps[i].data;
    info.data.RowPitch   = bitmaps[i].row_pitch;
    info.data.SlicePitch = bitmaps[i].row_pitch * bitmaps[i].height;
    _infos.emplace_back(std::move(info));
  }
}

void UploadBuffer::upload(ID3D12GraphicsCommandList1* cmd) noexcept
{
  // calculate required intermediate sizes
  auto intermediate_sizes = std::vector<uint32_t>{};
  intermediate_sizes.reserve(_infos.size());
  for (auto const& info : _infos)
    intermediate_sizes.emplace_back(
      align(GetRequiredIntermediateSize(info.image->handle(), 0, 1), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));

  // initialize buffer
  auto size = std::ranges::fold_left(intermediate_sizes, 0, std::plus<>{});
  if (_buffer.capacity() < size)
    _buffer.init(size, false);

  // copy bitmap data to image by upload buffer
  auto offset = uint32_t{};
  for (auto i = 0; i < _infos.size(); ++i)
  {
    copy(cmd, *_infos[i].image, _buffer.handle(), offset, _infos[i].data);
    offset += intermediate_sizes[i];
  }

  _infos.clear();
}

////////////////////////////////////////////////////////////////////////////////
///                             Copy Engine
////////////////////////////////////////////////////////////////////////////////

void CopyEngine::init() noexcept
{
  Engine::init(D3D12_COMMAND_LIST_TYPE_COPY);
}

void CopyEngine::destroy() noexcept
{
  Engine::destroy();
  for (auto slot : _slots)
    _slot_pool.free(slot);
}

auto CopyEngine::Slot::is_idle() const noexcept -> bool
{
  return g_copy_engine.fence_completed_value() >= fence_value;
}

auto CopyEngine::create_slot() noexcept -> SlotHandle
{
  auto handle = _slot_pool.alloc();
  _slot_pool[handle].cmd_alloc = g_core.create_cmd_alloc(D3D12_COMMAND_LIST_TYPE_COPY);
  return handle;
}

void CopyEngine::acquire_slot() noexcept
{
  if (auto it = std::ranges::find_if(_slots, [this](auto slot) { return _slot_pool[slot].is_idle(); });
      it != _slots.end())
  {
    _slot = *it;
  }
  else
  {
    _slots.emplace_back(create_slot());
    _slot = _slots.back();
  }
}

void CopyEngine::copy(std::vector<Bitmap> const& bitmaps, std::vector<Image*> const& images) noexcept
{
  auto& slot = _slot_pool[_slot];
  assert(slot.is_idle());
  slot.upload_buffer.add_images(images, bitmaps);
}

auto CopyEngine::submit_slot() noexcept -> uint64_t
{
  auto& slot = _slot_pool[_slot];
  assert(slot.is_idle());
  reset_cmd(slot.cmd_alloc.Get());
  slot.upload_buffer.upload(cmd());
  slot.fence_value = submit(); 
  return slot.fence_value;
}

}}
