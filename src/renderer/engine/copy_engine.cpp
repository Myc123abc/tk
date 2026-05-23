#include "copy_engine.hpp"
#include "../../util/align.hpp"
#include "../resource/image.hpp"

#include <algorithm>

namespace tk::renderer {

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
  if (_infos.empty()) return;

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

void CopyEngine::acquire_slot() noexcept
{
  _slots.acquire_slot();
}

auto CopyEngine::submit_slot() noexcept -> uint64_t
{
  _slots.slot()->data.upload(cmd());
  return _slots.submit_slot();
}

void CopyEngine::copy(std::vector<Bitmap> const& bitmaps, std::vector<Image*> const& images) noexcept
{
  auto slot = _slots.slot();
  assert(slot && _slots.is_idle(slot));
  slot->data.add_images(images, bitmaps);
}

}
