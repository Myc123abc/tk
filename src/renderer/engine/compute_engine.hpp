#pragma once

#include "engine.hpp"
#include "../../util/singleton.hpp"
#include "../resource/image.hpp"

#include <deque>
#include <vector>

namespace tk::renderer {

Singleton_Derive(ComputeEngine, g_comp_engine, Engine,
public:
  void init() noexcept { Engine::init(D3D12_COMMAND_LIST_TYPE_COMPUTE); }

  void acquire_slot() noexcept;

  [[nodiscard]]
  auto submit_slot() noexcept -> uint64_t;

  void blur(Image& src, Image& dst, float radius) noexcept;

  void update() noexcept;

private:
  auto get_tmp_img() noexcept -> std::pair<Image*, uint32_t>;

private:
  struct Slot
  {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_alloc;
    uint64_t                                       fence_value{};  

    auto is_idle() const noexcept -> bool;

    Slot() noexcept;
  };

  std::vector<Slot> _slots;
  Slot*             _slot{};

  struct BlurTmpImage
  {
    Image img;
    bool  in_use{};
  };
  std::vector<BlurTmpImage>                 _blur_tmp_images;
  std::deque<std::pair<uint32_t, uint64_t>> _used_blur_tmp_images;
)

}
