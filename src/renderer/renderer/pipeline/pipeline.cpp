#include "pipeline.hpp"
#include "util/error_handling.hpp"
#include "../../core.hpp"
#include "compiler.hpp"

#include <directx/d3dx12.h>

using namespace tk;
using namespace tk::renderer;
using namespace Microsoft::WRL;

namespace tk { namespace renderer {

void Pipeline::init_graphics(
    std::string_view shader,
    std::string_view vs,
    std::string_view ps,
    std::string_view include,
    ImageFormat      rtv_format,
    bool             use_blend,
    bool             use_depth_test
  ) noexcept
{
  _is_graphics_pipeline = true;

  auto compile_result = g_compiler.compile(shader, vs, ps, include);
  _root_params      = compile_result.root_params;
  _root_signature   = compile_result.root_signature;
  _resource_indices = compile_result.resource_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  
  auto render_target_formats = D3D12_RT_FORMAT_ARRAY{};
  render_target_formats.NumRenderTargets = 1;
  render_target_formats.RTFormats[0]     = dxgi_format(rtv_format);
  
  stream.pRootSignature        = _root_signature.Get();
  stream.InputLayout           = compile_result.input_layout_desc;
  stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  stream.VS                    = compile_result.vs;
  stream.PS                    = compile_result.ps;
  stream.RTVFormats            = render_target_formats;
    
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
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&_pipeline_state)),
          "failed to create pipeline state");
}

void Pipeline::init_compute(std::string_view shader, std::string_view cs, std::string_view include) noexcept
{
  auto compile_result = g_compiler.compile(shader, cs, include);
  _root_signature   = compile_result.root_signature;
  _resource_indices = compile_result.resource_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  stream.pRootSignature = _root_signature.Get();
  stream.CS             = compile_result.cs;

  auto pipeline_state_stream_desc = D3D12_PIPELINE_STATE_STREAM_DESC{ sizeof(stream), &stream };
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&_pipeline_state)),
          "failed to create pipeline state");
}

void Pipeline::bind(ID3D12GraphicsCommandList1* cmd) const noexcept
{
  cmd->SetPipelineState(_pipeline_state.Get());
  if (_is_graphics_pipeline)
  {
    cmd->SetGraphicsRootSignature(_root_signature.Get());
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  }
  else
    cmd->SetComputeRootSignature(_root_signature.Get());
}

void Pipeline::set_descriptors(ID3D12GraphicsCommandList1* cmd, std::initializer_list<std::pair<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>> handles) const noexcept
{
  for (auto const& [name, handle] : handles)
    set_descriptor(cmd, name, handle);
}

void Pipeline::set_descriptor(ID3D12GraphicsCommandList1* cmd, std::string_view name, D3D12_GPU_DESCRIPTOR_HANDLE handle) const noexcept
{
  if (_resource_indices.contains(name.data()) && handle.ptr)
  {
    _is_graphics_pipeline
      ? cmd->SetGraphicsRootDescriptorTable(_resource_indices.at(name.data()), handle)
      : cmd->SetComputeRootDescriptorTable(_resource_indices.at(name.data()), handle);
  }
}

}}
