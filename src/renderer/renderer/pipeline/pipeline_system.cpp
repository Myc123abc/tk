#include "pipeline_system.hpp"
#include "../../core.hpp"
#include "util/error_handling.hpp"

#include <ranges>

namespace tk::renderer {

void PipelineSystem::init() noexcept
{
  g_compiler.init();

  using enum DescriptorInfo::Type;

  auto res = generate_root_signature(
  {
    { constants,   "constants", 0, 0, false, sizeof(Constants) },
    { texture,     "image",     0, 0                           },
  }, true, true);
  _pipes.emplace(PipelineType::shape, Pipeline{ "assets/shader/ui/shape.hlsl", "vs", "ps", { "assets/shader/ui" }, RenderResource::Render_Target_Format, true, false, res });
  _pipes.emplace(PipelineType::image, Pipeline{ "assets/shader/ui/image.hlsl", "vs", "ps", { "assets/shader/ui" }, RenderResource::Render_Target_Format, true, false, res });
  _pipes.emplace(PipelineType::window_shadow, Pipeline{ "assets/shader/ui/window_shadow.hlsl", "vs", "ps", { "assets/shader/ui" }, RenderResource::Render_Target_Format, true, false, res });

  // res = generate_root_signature(
  // {
  //   { constants,  "constants", 0, 0, false, sizeof(BlurConstants) },
  //   { texture,    "src",       0, 0, true                         },
  //   { rw_texture, "dst",       0, 0, true                         },
  // });
  // _pipes.emplace(PipelineType::blur_horizontal_pass, Pipeline{ "assets/shader/blur.hlsl", "horizontal_pass", {}, res, {} });
  // _pipes.emplace(PipelineType::blur_vertical_pass,   Pipeline{ "assets/shader/blur.hlsl", "vertical_pass",   {}, res, {} });
}

auto PipelineSystem::find_root_param(std::span<CD3DX12_ROOT_PARAMETER1> params) const noexcept -> ID3D12RootSignature*
{
  auto it = std::ranges::find_if(_root_signatures, [&](auto const& pair)
  {
    auto const& cur_params = pair.second;
    if (cur_params.size() != params.size()) return false;
    for (auto i : std::views::iota(0u, cur_params.size()))
    {
      if (memcmp(&cur_params[i], &params[i], sizeof(CD3DX12_ROOT_PARAMETER1)))
        return false;
    }
    return true;
  });
  if (it == _root_signatures.end()) return {};
  return it->first.Get();
}

void PipelineSystem::Pipeline::init_graphics(
    std::string_view                       shader,
    std::string_view                       vs,
    std::string_view                       ps,
    std::vector<std::string_view> const&   includes,
    ImageFormat                            rtv_format,
    bool                                   use_blend,
    bool                                   use_depth_test,
    std::optional<RootSignatureResult>     res,
    std::unordered_set<std::string> const& volatile_descs
  ) noexcept
{
  auto compile_result = g_compiler.compile(shader, vs, ps, includes, res, volatile_descs);
  
  if (auto res = g_pipe_sys.find_root_param(compile_result.root_params))
    root_signature = res;
  else
    root_signature = g_pipe_sys._root_signatures.emplace_back(compile_result.root_signature, compile_result.root_params).first.Get();
  _root_param_idxs = compile_result.root_param_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  
  auto render_target_formats = D3D12_RT_FORMAT_ARRAY{};
  render_target_formats.NumRenderTargets = 1;
  render_target_formats.RTFormats[0]     = static_cast<DXGI_FORMAT>(rtv_format);
  
  stream.pRootSignature        = root_signature;
  stream.InputLayout           = compile_result.input_layout_desc;
  stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  stream.VS                    = compile_result.vs;
  stream.PS                    = compile_result.ps;
  stream.RTVFormats            = render_target_formats;
  
  // TODO: cull mode none only use 2D render now, 3D need to use back cull
  auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  rasterizer.DepthClipEnable = false;
  rasterizer.CullMode        = D3D12_CULL_MODE_NONE;
  stream.RasterizerState     = rasterizer;
    
  auto depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
  depth_stencil_desc.DepthEnable           = false;
  depth_stencil_desc.DepthBoundsTestEnable = false;
  if (use_depth_test)
  {
    // check feature support
    auto options = D3D12_FEATURE_DATA_D3D12_OPTIONS2{};
    err_if(g_core.device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options, sizeof(options)),
            "failed to get feature options");
    err_if(!options.DepthBoundsTestSupported, "unsupport depth bounds test");

    depth_stencil_desc.DepthBoundsTestEnable = true;
    stream.DSVFormat                         = DXGI_FORMAT_D32_FLOAT;
  }
  stream.DepthStencilState = depth_stencil_desc;
  
  auto  blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  auto& rt          = blend_state.RenderTarget[0];
  rt.BlendEnable           = use_blend;
  rt.SrcBlend              = D3D12_BLEND_SRC_ALPHA;
  rt.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
  rt.BlendOp               = D3D12_BLEND_OP_ADD;
  rt.SrcBlendAlpha         = D3D12_BLEND_ONE;
  rt.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
  rt.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
  rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  stream.BlendState = blend_state;
  
  auto pipeline_state_stream_desc = D3D12_PIPELINE_STATE_STREAM_DESC{ sizeof(stream), &stream };
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipe_state)),
          "failed to create pipeline state");
}

void PipelineSystem::Pipeline::init_compute(std::string_view shader, std::string_view cs, std::vector<std::string_view> const& includes,
  std::optional<RootSignatureResult> res, std::unordered_set<std::string> const& volatile_descs) noexcept
{
  auto compile_result = g_compiler.compile(shader, cs, includes, res, volatile_descs);
  if (auto res = g_pipe_sys.find_root_param(compile_result.root_params))
    root_signature = res;
  else
    root_signature = g_pipe_sys._root_signatures.emplace_back(compile_result.root_signature, compile_result.root_params).first.Get();
  _root_param_idxs = compile_result.root_param_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  stream.pRootSignature = root_signature;
  stream.CS             = compile_result.cs;

  auto pipeline_state_stream_desc = D3D12_PIPELINE_STATE_STREAM_DESC{ sizeof(stream), &stream };
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipe_state)),
          "failed to create pipeline state");
}

}
