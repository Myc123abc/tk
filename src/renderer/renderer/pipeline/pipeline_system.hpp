#pragma once

#include "../../../util/singleton.hpp"
#include "../../resource/render_resource.hpp"
#include "compiler.hpp"

#include <span>

namespace tk::renderer {

enum class PipelineType
{
  shape,
  image,
  // TODO: split text render from sdf
  blur_horizontal_pass,
  blur_vertical_pass,
  window_shadow,
};

Singleton(PipelineSystem, g_pipe_sys,
public:
  void init() noexcept;

  auto pipe(PipelineType type) noexcept { return &_pipes[type]; }

private:
  auto find_root_param(std::span<CD3DX12_ROOT_PARAMETER1> params) const noexcept -> ID3D12RootSignature*;

  struct Pipeline
  {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipe_state{};
    ID3D12RootSignature*                        root_signature{};
    D3D_PRIMITIVE_TOPOLOGY                      primive_topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };

    Pipeline() = default;

    Pipeline(
      std::string_view                       shader,
      std::string_view                       vs,
      std::string_view                       ps,
      std::vector<std::string_view> const&   includes,
      ImageFormat                            rtv_format,
      bool                                   use_blend      = false,
      bool                                   use_depth_test = false,
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}
    ) noexcept
    {
      init_graphics(shader, vs, ps, includes, rtv_format, use_blend, use_depth_test, res, volatile_descs);
    }

    Pipeline(
      std::string_view                       shader,
      std::string_view                       cs,
      std::vector<std::string_view> const&   includes       = {},
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}) noexcept
    {
      init_compute(shader, cs, includes, res, volatile_descs);
    }

    void init_graphics(
      std::string_view                       shader,
      std::string_view                       vs,
      std::string_view                       ps,
      std::vector<std::string_view> const&   includes,
      ImageFormat                            rtv_format,
      bool                                   use_blend      = false,
      bool                                   use_depth_test = false,
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}
    ) noexcept;
  
    void init_compute(
      std::string_view                       shader,
      std::string_view                       cs,
      std::vector<std::string_view> const&   includes       = {},
      std::optional<RootSignatureResult>     res            = {},
      std::unordered_set<std::string> const& volatile_descs = {}) noexcept;
  
    auto root_param_idx(std::string_view name) noexcept { return _root_param_idxs[name.data()]; }
  private:
    std::unordered_map<std::string, uint32_t> _root_param_idxs;
  };
  std::unordered_map<PipelineType, Pipeline> _pipes;
  std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, std::vector<CD3DX12_ROOT_PARAMETER1>>> _root_signatures;
)

}
