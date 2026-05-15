#include "pipeline_system.hpp"
#include "../core.hpp"
#include "util/error_handling.hpp"
#include "../resource/shader_type.hpp"

#include <ranges>

namespace tk::renderer {

void PipelineSystem::init() noexcept
{
  g_compiler.init();

  using enum DescriptorInfo::Type;

  auto res = generate_root_signature(
  {
    { constants, "constants",  0, 0, false, sizeof(Constants) },
    { texture,   "image",      0, 0, true                     },
    { texture,   "mask_image", 0, 1, true                     },
  }, true, true);

  auto info = PipelineCreateInfo{};
  info.shader                  = "assets/shader/ui/ui.hlsl";
  info.graphics.vs             = "vs";
  info.graphics.ps             = "ps";
  info.includes                = { "assets/shader/ui" };
  info.graphics.rtv_format     = RenderResource::Render_Target_Format;
  info.graphics.blend          = BlendState::Default();
  info.root_signature_result   = res;
  _pipes.emplace(PipelineType::ui, info);

  info.shader = "assets/shader/ui/composite_write.hlsl";
  _pipes.emplace(PipelineType::composite_write, info);

  auto stencil = StencilState{};
  stencil.op = StencilOp::replace;
  info.graphics.stencil = stencil;
  info.graphics.blend   = {};
  _pipes.emplace(PipelineType::stencil_replace_write, info);

  stencil.op            = StencilOp::keep;
  stencil.comp          = CompFunc::equal;
  stencil.write_color   = true;
  info.graphics.stencil = stencil;
  info.graphics.blend   = BlendState::Default();
  _pipes.emplace(PipelineType::stencil_equal_test, info);

  stencil.comp          = CompFunc::not_equal;
  info.graphics.stencil = stencil;
  _pipes.emplace(PipelineType::stencil_not_equal_test, info);

  info.graphics.stencil = {};
  info.shader = "assets/shader/ui/window_shadow.hlsl";
  _pipes.emplace(PipelineType::window_shadow, info);

  info.shader = "assets/shader/ui/discard_draw.hlsl";
  _pipes.emplace(PipelineType::discard_draw, info);

  info.shader = "assets/shader/ui/mask_write.hlsl";
  info.graphics.rtv_format = ImageFormat::r8_unorm;
  info.graphics.blend      = BlendState::Max();
  _pipes.emplace(PipelineType::mask_write_max, info);

  info.graphics.blend = BlendState::Add();
  _pipes.emplace(PipelineType::mask_write_add, info);

  info.graphics.stencil = {};
  info.shader = "assets/shader/ui/window_shadow.hlsl";
  info.graphics.blend = BlendState::Default();
  _pipes.emplace(PipelineType::window_shadow, info);

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

void PipelineSystem::Pipeline::init_graphics(PipelineCreateInfo const& info) noexcept
{
  auto compile_result = g_compiler.compile(info.shader, info.graphics.vs, info.graphics.ps, info.includes, info.root_signature_result, info.volatile_descs);
  
  if (auto res = g_pipe_sys.find_root_param(compile_result.root_params))
    root_signature = res;
  else
    root_signature = g_pipe_sys._root_signatures.emplace_back(compile_result.root_signature, compile_result.root_params).first.Get();
  _root_param_idxs = compile_result.root_param_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  
  auto render_target_formats = D3D12_RT_FORMAT_ARRAY{};
  render_target_formats.NumRenderTargets = 1;
  render_target_formats.RTFormats[0]     = static_cast<DXGI_FORMAT>(info.graphics.rtv_format);
  
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

  // depth and stencil setting  
  auto depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
  depth_stencil_desc.DepthEnable           = false;
  depth_stencil_desc.DepthBoundsTestEnable = false;
  if (info.graphics.use_depth_test)
  {
    depth_stencil_desc.DepthEnable = true;

    // check feature support
    auto options = D3D12_FEATURE_DATA_D3D12_OPTIONS2{};
    err_if(g_core.device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options, sizeof(options)),
            "failed to get feature options");
    err_if(!options.DepthBoundsTestSupported, "unsupport depth bounds test");
    depth_stencil_desc.DepthBoundsTestEnable = true;
  }
  if (info.graphics.stencil)
  {
    auto stencil = info.graphics.stencil.value();
    depth_stencil_desc.StencilEnable = true;
    depth_stencil_desc.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(stencil.op);
    depth_stencil_desc.FrontFace.StencilFunc   = static_cast<D3D12_COMPARISON_FUNC>(stencil.comp);
    depth_stencil_desc.BackFace = depth_stencil_desc.FrontFace;
  }
  if (info.graphics.use_depth_test || info.graphics.stencil)
    stream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  stream.DepthStencilState = depth_stencil_desc;
  
  auto  blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  auto& rt          = blend_state.RenderTarget[0];
  if (rt.BlendEnable = info.graphics.blend.has_value(); rt.BlendEnable)
  {
    auto blend = info.graphics.blend.value();
    rt.SrcBlend       = static_cast<D3D12_BLEND>(blend.src);
    rt.DestBlend      = static_cast<D3D12_BLEND>(blend.dst);
    rt.BlendOp        = static_cast<D3D12_BLEND_OP>(blend.op);
    rt.SrcBlendAlpha  = static_cast<D3D12_BLEND>(blend.src_alpha);
    rt.DestBlendAlpha = static_cast<D3D12_BLEND>(blend.dst_alpha);
    rt.BlendOpAlpha   = static_cast<D3D12_BLEND_OP>(blend.op_alpha);
  }
  if (info.graphics.stencil)
    rt.RenderTargetWriteMask = info.graphics.stencil->write_color ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
  else
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  stream.BlendState = blend_state;
  
  auto pipeline_state_stream_desc = D3D12_PIPELINE_STATE_STREAM_DESC{ sizeof(stream), &stream };
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipe_state)),
          "failed to create pipeline state");
}

void PipelineSystem::Pipeline::init_compute(PipelineCreateInfo const& info) noexcept
{
  auto compile_result = g_compiler.compile(info.shader, info.compute.cs, info.includes, info.root_signature_result, info.volatile_descs);
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
