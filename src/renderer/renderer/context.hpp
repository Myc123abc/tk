#pragma once

#include "../../util/singleton.hpp"
#include "../resource/image.hpp"

namespace tk::renderer {

Singleton(Context, g_ctx,
public:
  void set_cmd(ID3D12GraphicsCommandList1* cmd) noexcept;
  void set_pipe(ID3D12PipelineState* pipe_state) noexcept;
  void set_graphics_root_signature(ID3D12RootSignature* root_signature) noexcept;
  void set_compute_root_signature(ID3D12RootSignature* root_signature) noexcept;
  void set_graphics_descriptor(uint32_t root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept;
  void set_compute_descriptor(uint32_t root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept;
  void set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY primitive_topology) noexcept;
  void set_scissor_rect(RECT rect) noexcept;
  void set_render_target(Image& img) const noexcept;
  void draw(uint32_t count) const noexcept;
  void draw(uint32_t start_idx, uint32_t size) const noexcept;
  void dispatch(uint32_t x, uint32_t y, uint32_t z) const noexcept;

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void set_graphics_constants(uint32_t root_param_idx, T const& constants) noexcept;

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void set_compute_constants(uint32_t root_param_idx, T const& constants);

private:
  using DescMapType = std::unordered_map<uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE>;

  ID3D12GraphicsCommandList1* _cmd{};
  ID3D12PipelineState*        _pipe_state{};
  ID3D12RootSignature*        _graphics_root_signature{};
  ID3D12RootSignature*        _compute_root_signature{};
  DescMapType                 _graphics_descriptors;
  DescMapType                 _compute_descriptors;
  uint32_t                    _graphics_constants_root_param_idx{};
  std::vector<uint8_t>        _graphics_constants;
  uint32_t                    _compute_constants_root_param_idx{};
  std::vector<uint8_t>        _compute_constants;
  D3D_PRIMITIVE_TOPOLOGY      _primitive_topology{};
  RECT                        _scissor_rect{};
)

template <typename T>
requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
void Context::set_graphics_constants(uint32_t root_param_idx, T const& constants) noexcept
{
  auto size = sizeof(constants);
  if (_graphics_constants_root_param_idx != root_param_idx ||
      _graphics_constants.size()         != size           ||
      memcmp(_graphics_constants.data(), &constants, size))
  {
    _graphics_constants_root_param_idx = root_param_idx;
    _graphics_constants.resize(size);
    memcpy(_graphics_constants.data(), &constants, size);
    _cmd->SetGraphicsRoot32BitConstants(root_param_idx, _graphics_constants.size() / 4, _graphics_constants.data(), 0);
  }
}

template <typename T>
requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
void Context::set_compute_constants(uint32_t root_param_idx, T const& constants)
{
  auto size = sizeof(constants);
  if (_compute_constants_root_param_idx != root_param_idx ||
      _compute_constants.size()         != size           ||
      memcmp(_compute_constants.data(), &constants, size))
  {
    _compute_constants_root_param_idx = root_param_idx;
    _compute_constants.resize(size);
    memcpy(_compute_constants.data(), &constants, size);
    _cmd->SetComputeRoot32BitConstants(root_param_idx, _compute_constants.size() / 4, _compute_constants.data(), 0);
  }
}

}
