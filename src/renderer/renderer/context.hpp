#pragma once

#include "../../util/singleton.hpp"
#include "pipeline/pipeline_system.hpp"
#include "util/rect.hpp"

#include <d3d12.h>
#include <unordered_map>
#include <optional>

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
  void set_scissor_rect(Rect rect) noexcept;
  void set_stencil_value(uint32_t value) noexcept;
  void set_render_target(Image* render_tareget_image, Image* depth_stencil_image) noexcept;
  void draw(uint32_t count) const noexcept;
  void draw(uint32_t start_idx, uint32_t size) const noexcept;
  void dispatch(uint32_t x, uint32_t y, uint32_t z) const noexcept;

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void set_graphics_constants(uint32_t root_param_idx, T const& constants) noexcept;

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void set_compute_constants(uint32_t root_param_idx, T const& constants);

  struct DescriptorInfo
  {
    std::string_view            name;
    D3D12_GPU_DESCRIPTOR_HANDLE handle;
  };

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void graphics_pipe_set(PipelineType type, Rect scissor_rect, std::string_view constants_name, T const& constants, std::initializer_list<DescriptorInfo> descs = {}) noexcept;

  template <typename T>
  requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
  void graphics_draw(PipelineType type, Image* render_target, Image* depth_stencil, ui::DrawCmd const& cmd,
    std::string_view constants_name, T const& constants, std::initializer_list<DescriptorInfo> descs = {}) noexcept;

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
  Rect                        _scissor_rect{};
  std::optional<uint32_t>     _stencil_value{};
  D3D12_CPU_DESCRIPTOR_HANDLE _render_target{};
  D3D12_CPU_DESCRIPTOR_HANDLE _depth_stencil{};
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

template <typename T>
requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
void Context::graphics_pipe_set(PipelineType type, Rect scissor_rect, std::string_view constants_name, T const& constants, std::initializer_list<DescriptorInfo> descs) noexcept
{
  auto pipe = g_pipe_sys.pipe(type);
  set_pipe(pipe->pipe_state.Get());
  set_graphics_root_signature(pipe->root_signature);
  set_primitive_topology(pipe->primive_topology);
  set_graphics_constants(pipe->root_param_idx(constants_name), constants);
  for (auto const& [name, handle] : descs)
    set_graphics_descriptor(pipe->root_param_idx(name), handle);
  set_scissor_rect(scissor_rect);
}

template <typename T>
requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
void Context::graphics_draw(PipelineType type, Image* render_target, Image* depth_stencil, ui::DrawCmd const& cmd,
  std::string_view constants_name, T const& constants, std::initializer_list<DescriptorInfo> descs) noexcept
{
  set_render_target(render_target, depth_stencil);
  graphics_pipe_set(type, cmd.ui.scissor_rect, constants_name, constants, descs);
  draw(cmd.ui.idx_beg, cmd.ui.idx_size);
}

}
