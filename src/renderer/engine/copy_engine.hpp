#pragma once

#include "engine.hpp"
#include "../config.hpp"
#include "../../util/object_pool.hpp"
#include "../resource/image.hpp"
#include "../resource/buffer.hpp"

namespace tk { namespace renderer {

class UploadBuffer
{
public:
  void add_images(std::vector<ImageHandle> const& image_handles, std::vector<BitmapView> const& bitmaps) noexcept;
  void upload(ID3D12GraphicsCommandList1* cmd) noexcept;

private:
  struct Info
  {
    D3D12_SUBRESOURCE_DATA data{};
    ImageHandle            handle;
  };

  Buffer            _buffer;
  std::vector<Info> _infos;
};

// TODO: don't use inherate?
class CopyEngine final : public Engine
{
public:
  static auto instance() noexcept -> CopyEngine&
  {
    static CopyEngine instance;
    return instance;
  }

  void init() noexcept;
  void destroy() noexcept;

  void acquire_slot() noexcept;

  void copy(std::vector<BitmapView> const& bitmaps, std::vector<ImageHandle> const& image_handles) noexcept;

  auto submit_slot() noexcept -> uint64_t;

private:
  struct Slot
  {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
    UploadBuffer                                   upload_buffer;
    uint64_t                                       fence_value{};

    auto is_idle() const noexcept -> bool;

    Slot() noexcept = default;
  };

  using SlotPoolType = ObjectPool<Slot, Copy_Engine_Slot_Pool_Init_Capacity>;
  using SlotHandle   = SlotPoolType::Handle;

  auto create_slot() noexcept -> SlotHandle;

  SlotPoolType            _slot_pool;
  std::vector<SlotHandle> _slots;
  SlotHandle              _slot{};
};

inline static auto& g_copy_engine{ CopyEngine::instance() };

}}
