#pragma once

#include "engine.hpp"
#include "../resource/image.hpp"
#include "../resource/buffer.hpp"

namespace tk { namespace renderer {

class UploadBuffer
{
public:
  void add_images(std::vector<Image*> const& images, std::vector<Bitmap> const& bitmaps) noexcept;
  void upload(ID3D12GraphicsCommandList1* cmd) noexcept;

private:
  struct Info
  {
    D3D12_SUBRESOURCE_DATA data{};
    Image*                 image{};
  };

  Buffer            _buffer;
  std::vector<Info> _infos;
};

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

  void copy(std::vector<Bitmap> const& bitmaps, std::vector<Image*> const& images) noexcept;

  [[nodiscard]]
  auto submit_slot() noexcept -> uint64_t;

private:
  struct Slot
  {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
    UploadBuffer                                   upload_buffer;
    uint64_t                                       fence_value{};

    auto is_idle() const noexcept -> bool;

    Slot() noexcept;
  };

  std::vector<Slot> _slots;
  Slot*             _slot{};
};

inline static auto& g_copy_engine{ CopyEngine::instance() };

}}
