#include "pipeline_system.hpp"
#include "../core.hpp"
#include "../resource/render_resource.hpp"
#include "tk/error_handling.hpp"
#include "../resource/shader_type.hpp"

namespace tk::renderer {

void PipelineSystem::create_graphics_pipeline(GraphicsPipelineInfo const& info) noexcept
{
  auto create_info = PipelineCreateInfo{};
  create_info.shader                = info.shader;
  create_info.includes              = info.includes;
  create_info.root_signature_result = info.root_sign;

  create_info.info = PipelineCreateInfo::Graphics{};
  auto& graphics = create_info.info.get<PipelineCreateInfo::Graphics>();
  graphics.vs         = info.vs;
  graphics.ps         = info.ps;
  graphics.rtv_format = info.rt_fmt;
  graphics.blend      = info.blend;
  _pipes.emplace(info.type, create_info);
}

void PipelineSystem::create_compute_pipeline(ComputePipelineInfo const& info) noexcept
{
  auto create_info = PipelineCreateInfo{};
  create_info.shader                = info.shader;
  create_info.includes              = info.includes;
  create_info.root_signature_result = info.root_sign;

  create_info.info = PipelineCreateInfo::Compute{};
  auto& compute = create_info.info.get<PipelineCreateInfo::Compute>();
  compute.cs = info.cs;
  _pipes.emplace(info.type, create_info);
}
  
void PipelineSystem::init() noexcept
{
  g_compiler.init();

  using enum DescriptorInfo::Type;

  auto ui_res = generate_root_signature(
  {
    { constants, "constants",       0, 0, false, sizeof(Constants) },
    { textures,  "images",          0, 0, true                     },
    { texture,   "mask_image",      0, 1, true                     },
    { texture,   "composite_image", 0, 2, true                     },
  }, true, true);
  auto image_scale_res = generate_root_signature(
  {
    { constants, "Constants", 0, 0, false, sizeof(float2) },
    { texture,   "img",       0, 0, true                  },
  }, true, true);
  create_graphics_pipelines({
  {
    .type      = PipelineType::ui,
    .shader    = "assets/shader/ui/ui.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = { "assets/shader/ui" },
    .rt_fmt    = RenderResource::Render_Target_Format,
    .blend     = BlendState::Default(),
    .root_sign = ui_res,
  },
  {
    .type      = PipelineType::window_shadow,
    .shader    = "assets/shader/ui/window_shadow.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = { "assets/shader/ui" },
    .rt_fmt    = RenderResource::Render_Target_Format,
    .blend     = BlendState::Default(),
    .root_sign = ui_res,
  },
  {
    .type      = PipelineType::discard_draw,
    .shader    = "assets/shader/ui/discard_draw.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = { "assets/shader/ui" },
    .rt_fmt    = RenderResource::Render_Target_Format,
    .blend     = BlendState::Default(),
    .root_sign = ui_res,
  },
  {
    .type      = PipelineType::mask_write_max,
    .shader    = "assets/shader/ui/mask_write.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = { "assets/shader/ui" },
    .rt_fmt    = ImageFormat::r8_unorm,
    .blend     = BlendState::Max(),
    .root_sign = ui_res,
  },
  {
    .type      = PipelineType::mask_write_add,
    .shader    = "assets/shader/ui/mask_write.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = { "assets/shader/ui" },
    .rt_fmt    = ImageFormat::r8_unorm,
    .blend     = BlendState::Add(),
    .root_sign = ui_res,
  },
  {
    .type      = PipelineType::image_scale,
    .shader    = "assets/shader/image_scale.hlsl",
    .vs        = "vs",
    .ps        = "ps",
    .includes  = {},
    .rt_fmt    = RenderResource::Render_Target_Format,
    .blend     = {},
    .root_sign = image_scale_res,
  }});

  auto mipmap_res = generate_root_signature(
  {
    { constants,  "constants", 0, 0, false, sizeof(float2) },
    { texture,    "src",       0, 0, true,                 },
    { rw_texture, "dst",       0, 0, true,                 },
  }, false, true);
  auto blur_res = generate_root_signature(
  {
    { constants,  "constants", 0, 0, false, sizeof(BlurConstants) },
    { texture,    "src",       0, 0, true                         },
    { rw_texture, "dst",       0, 0, true                         },
  });
  create_compute_pipelines({
  {
    .type      = PipelineType::mipmap,
    .shader    = "assets/shader/mipmap.hlsl",
    .cs        = "main",
    .includes  = {},
    .root_sign = mipmap_res,
  },
  {
    .type      = PipelineType::blur_horizontal_pass,
    .shader    = "assets/shader/blur.hlsl",
    .cs        = "horizontal_pass",
    .includes  = {},
    .root_sign = blur_res,
  },
  {
    .type      = PipelineType::blur_vertical_pass,
    .shader    = "assets/shader/blur.hlsl",
    .cs        = "vertical_pass",
    .includes  = {},
    .root_sign = blur_res,
  }});
}

auto PipelineSystem::find_root_param(std::span<CD3DX12_ROOT_PARAMETER1> params) const noexcept -> ID3D12RootSignature*
{
  auto it = std::ranges::find_if(_root_signatures, [&](auto const& pair)
  {
    auto const& cur_params = pair.second;
    if (cur_params.size() != params.size()) return false;
    for (auto i = 0; i < cur_params.size(); ++i)
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
  auto const& graphics = info.info.get<PipelineCreateInfo::Graphics>();
  auto compile_result = g_compiler.compile(info.shader, graphics.vs, graphics.ps, info.includes, info.root_signature_result, info.volatile_descs);
  
  if (auto res = g_pipe_sys.find_root_param(compile_result.root_params))
    root_signature = res;
  else
    root_signature = g_pipe_sys._root_signatures.emplace_back(compile_result.root_signature, compile_result.root_params).first.Get();
  _root_param_idxs = compile_result.root_param_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  
  auto render_target_formats = D3D12_RT_FORMAT_ARRAY{};
  render_target_formats.NumRenderTargets = 1;
  render_target_formats.RTFormats[0]     = static_cast<DXGI_FORMAT>(graphics.rtv_format);
  
  stream.pRootSignature        = root_signature;
  stream.InputLayout           = compile_result.input_layout_desc;
  stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  stream.VS                    = compile_result.vs;
  stream.PS                    = compile_result.ps;
  stream.RTVFormats            = render_target_formats;
  
  auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  rasterizer.DepthClipEnable = false;
  rasterizer.CullMode        = D3D12_CULL_MODE_NONE;
  stream.RasterizerState     = rasterizer;

  // depth and stencil setting  
  auto depth_stencil_desc = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
  depth_stencil_desc.DepthEnable           = false;
  depth_stencil_desc.DepthWriteMask        = D3D12_DEPTH_WRITE_MASK_ZERO;
  depth_stencil_desc.DepthBoundsTestEnable = false;
  if (graphics.use_depth_test)
  {
    depth_stencil_desc.DepthEnable    = true;
    depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    // check feature support
    auto options = D3D12_FEATURE_DATA_D3D12_OPTIONS2{};
    err_if(g_core.device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options, sizeof(options)),
            "failed to get feature options");
    err_if(!options.DepthBoundsTestSupported, "unsupport depth bounds test");
    depth_stencil_desc.DepthBoundsTestEnable = true;
  }
  if (graphics.stencil)
  {
    auto stencil = graphics.stencil.value();
    depth_stencil_desc.StencilEnable = true;
    depth_stencil_desc.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(stencil.op);
    depth_stencil_desc.FrontFace.StencilFunc   = static_cast<D3D12_COMPARISON_FUNC>(stencil.comp);
    depth_stencil_desc.BackFace = depth_stencil_desc.FrontFace;
  }
  if (graphics.use_depth_test || graphics.stencil)
    stream.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  stream.DepthStencilState = depth_stencil_desc;
  
  auto  blend_state = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  auto& rt          = blend_state.RenderTarget[0];
  if (rt.BlendEnable = graphics.blend.has_value(); rt.BlendEnable)
  {
    auto blend = graphics.blend.value();
    rt.SrcBlend       = static_cast<D3D12_BLEND>(blend.src);
    rt.DestBlend      = static_cast<D3D12_BLEND>(blend.dst);
    rt.BlendOp        = static_cast<D3D12_BLEND_OP>(blend.op);
    rt.SrcBlendAlpha  = static_cast<D3D12_BLEND>(blend.src_alpha);
    rt.DestBlendAlpha = static_cast<D3D12_BLEND>(blend.dst_alpha);
    rt.BlendOpAlpha   = static_cast<D3D12_BLEND_OP>(blend.op_alpha);
  }
  if (graphics.stencil)
    rt.RenderTargetWriteMask = graphics.stencil->write_color ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
  else
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  stream.BlendState = blend_state;
  
  auto pipeline_state_stream_desc = D3D12_PIPELINE_STATE_STREAM_DESC{ sizeof(stream), &stream };
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipe_state)),
          "failed to create pipeline state");
}

void PipelineSystem::Pipeline::init_compute(PipelineCreateInfo const& info) noexcept
{
  auto const& compute = info.info.get<PipelineCreateInfo::Compute>();
  auto compile_result = g_compiler.compile(info.shader, compute.cs, info.includes, info.root_signature_result, info.volatile_descs);
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
