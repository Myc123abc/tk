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
  blur_horizontal_pass,
  blur_vertical_pass,
  window_shadow,
  stencil_write,
  stencil_test,
};

enum class StencilOp
{
  keep     = D3D12_STENCIL_OP_KEEP,
  zero     = D3D12_STENCIL_OP_ZERO,
  replace  = D3D12_STENCIL_OP_REPLACE,
  incr_sat = D3D12_STENCIL_OP_INCR_SAT,
  decr_sat = D3D12_STENCIL_OP_DECR_SAT,
  invert   = D3D12_STENCIL_OP_INVERT,
  incr     = D3D12_STENCIL_OP_INCR,
  decr     = D3D12_STENCIL_OP_DECR,
};

enum class CompFunc
{
  none          = D3D12_COMPARISON_FUNC_NONE,
  never         = D3D12_COMPARISON_FUNC_NEVER,
  less          = D3D12_COMPARISON_FUNC_LESS,
  equal         = D3D12_COMPARISON_FUNC_EQUAL,
  less_equal    = D3D12_COMPARISON_FUNC_LESS_EQUAL,
  greater       = D3D12_COMPARISON_FUNC_GREATER,
  not_equal     = D3D12_COMPARISON_FUNC_NOT_EQUAL,
  greater_equal = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
  always        = D3D12_COMPARISON_FUNC_ALWAYS,
};

struct StencilState
{
  StencilOp op          = StencilOp::keep;
  CompFunc  comp        = CompFunc::always;
  bool      write_color = false;
};

Singleton(PipelineSystem, g_pipe_sys,
public:
  void init() noexcept;

  auto pipe(PipelineType type) noexcept { return &_pipes[type]; }

private:
  auto find_root_param(std::span<CD3DX12_ROOT_PARAMETER1> params) const noexcept -> ID3D12RootSignature*;

  struct PipelineCreateInfo
  {
    std::string_view                     shader;
    std::vector<std::string_view>        includes;
    std::optional<RootSignatureResult>   root_signature_result;
    std::unordered_set<std::string_view> volatile_descs;

    union
    {
      struct
      {
        std::string_view            vs;
        std::string_view            ps;
        ImageFormat                 rtv_format;
        bool                        use_blend{};
        bool                        use_depth_test{};
        std::optional<StencilState> stencil{};
      } graphics;

      struct
      {
        std::string_view cs;
      } compute;
    };
  };

  struct Pipeline
  {
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipe_state{};
    ID3D12RootSignature*                        root_signature{};
    D3D_PRIMITIVE_TOPOLOGY                      primive_topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };

    Pipeline() = default;

    Pipeline(PipelineCreateInfo const& info) noexcept
    {
      if (info.graphics.vs.empty())
        init_compute(info);
      else
        init_graphics(info);
    }

    void init_graphics(PipelineCreateInfo const& info) noexcept;
    void init_compute(PipelineCreateInfo const& info) noexcept;
  
    auto root_param_idx(std::string_view name) noexcept { return _root_param_idxs[name.data()]; }
  private:
    std::unordered_map<std::string, uint32_t> _root_param_idxs;
  };
  std::unordered_map<PipelineType, Pipeline> _pipes;
  std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D12RootSignature>, std::vector<CD3DX12_ROOT_PARAMETER1>>> _root_signatures;
)

}
