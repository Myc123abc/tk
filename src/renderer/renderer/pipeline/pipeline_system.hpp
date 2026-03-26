#pragma once

#include "../../../util/singleton.hpp"
#include "../../resource/render_resource.hpp"
#include "compiler.hpp"

#include <span>

namespace tk::renderer {

enum class PipelineType
{
  sdf,
  image,
  // TODO: split text render from sdf
  blur_horizontal_pass,
  blur_vertical_pass,
  window_shadow,
};

Singleton(PipelineSystem, g_pipe_sys,
public:
  void init() noexcept;

  void render(RenderResource& res, class RenderData& data) noexcept;

  auto ctx() noexcept { return &_ctx; }

  auto pipe(PipelineType type) const noexcept { return &_pipes.at(type); }

private:
  auto find_root_param(std::span<CD3DX12_ROOT_PARAMETER1> params) const noexcept -> ID3D12RootSignature*;

  struct Pipeline
  {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipe_state{};
    ID3D12RootSignature*                        root_signature{};
    D3D_PRIMITIVE_TOPOLOGY                      primive_topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };

    Pipeline(
      std::string_view                       shader,
      std::string_view                       vs,
      std::string_view                       ps,
      std::string_view                       include,
      ImageFormat                            rtv_format,
      bool                                   use_blend      = false,
      bool                                   use_depth_test = false,
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}
    ) noexcept
    {
      init_graphics(shader, vs, ps, include, rtv_format, use_blend, use_depth_test, res, volatile_descs);
    }

    Pipeline(
      std::string_view                       shader,
      std::string_view                       cs,
      std::string_view                       include = {},
      std::optional<RootSignatureResult>     res = {},
      std::unordered_set<std::string> const& volatile_descs = {}) noexcept
    {
      init_compute(shader, cs, include, res, volatile_descs);
    }

    void init_graphics(
      std::string_view                       shader,
      std::string_view                       vs,
      std::string_view                       ps,
      std::string_view                       include,
      ImageFormat                            rtv_format,
      bool                                   use_blend      = false,
      bool                                   use_depth_test = false,
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}
    ) noexcept;
  
    void init_compute(
      std::string_view                       shader,
      std::string_view                       cs,
      std::string_view                       include = {},
      std::optional<RootSignatureResult>     res = {},
      std::unordered_set<std::string> const& volatile_descs = {}) noexcept;
  
    auto root_param_idx(std::string_view name) const noexcept { return _root_param_idxs.at(name.data()); }
  private:
    std::unordered_map<std::string, uint32_t> _root_param_idxs;
  };
  std::unordered_map<PipelineType, Pipeline> _pipes;
  std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, std::vector<CD3DX12_ROOT_PARAMETER1>>> _root_signatures;

  struct Context
  {
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
  } _ctx;
)

template <typename T>
requires std::is_trivially_copyable_v<T> && (sizeof(T) % 4 == 0)
void PipelineSystem::Context::set_graphics_constants(uint32_t root_param_idx, T const& constants) noexcept
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
void PipelineSystem::Context::set_compute_constants(uint32_t root_param_idx, T const& constants)
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
