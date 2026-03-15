#include "pipeline_system.hpp"
#include "../../engine/graphics_engine.hpp"
#include "../../resource/render_data.hpp"
#include "../../core.hpp"

#include <ranges>

namespace tk { namespace renderer {

void PipelineSystem::init() noexcept
{
  g_compiler.init();

  using enum DescriptorInfo::Type;
  auto res = generate_root_signature(
  {
    { constants,  "constants", 0, 0, sizeof(Constants) },
    { bytebuffer, "buffer",    0, 0                    },
    { texture,    "image",     0, 1                    },
    { textures,   "images",    0, 2                    }
  }, true);

  _pipes.emplace(PipelineType::sdf, Pipeline{ "assets/shader/sdf.hlsl", "vs", "ps", "assets/shader", RenderResource::Render_Target_Format, true, false, res });
  _pipes.emplace(PipelineType::image, Pipeline{ "assets/shader/image.hlsl", "vs", "ps", "assets/shader", RenderResource::Render_Target_Format, true, false, res });
  // _mipmap_pipeline.init_compute("assets/shader/mipmap.hlsl", "main");
}

void PipelineSystem::render(RenderResource& res, RenderData& data) noexcept
{
  auto  cmd   = g_graphics_engine.cmd();
  auto& frame = res.current_frame();

  _ctx.set_cmd(cmd);

  // upload data to buffer
  frame.buffer.clear().upload(cmd, data);

  // pop first scissor rect
  auto scissor_rect = data.pop_scissor_rect();

  // render data
  for (auto const& draw_data : data.draw_datas)
  {
    if (draw_data.pipe_type == PipelineType::sdf)
    {
      auto& pipe = _pipes.at(PipelineType::sdf);

      _ctx.set_pipe(pipe.pipe_state.Get());
      _ctx.set_graphics_root_signature(pipe.root_signature);
      _ctx.set_primitive_topology(pipe.primive_topology);
      _ctx.set_graphics_descriptor(pipe.root_param_idx("buffer"), frame.buffer.shape_properties_gpu_handle());
      _ctx.set_graphics_constants(pipe.root_param_idx("constants"), Constants
      {
        .window_extent = frame.image.extent(),
        .window_pos    = data.resizing_window_pos
      });

      // draw data can be split by different scissor rect
      _ctx.set_scissor_rect(scissor_rect.rect);

      auto indices_start      = draw_data.indices_start;
      auto indices_size       = draw_data.indices_size;
      auto total_indices_size = draw_data.indices_start + draw_data.indices_size;
label_draw_call_again:
      if (total_indices_size < scissor_rect.next_indices_idx)
      {
        _ctx.draw(indices_start, indices_size);
        continue;
      }
      if (total_indices_size == scissor_rect.next_indices_idx)
      {
        _ctx.draw(indices_start, indices_size);
        assert(data.is_scissor_rect_empty());
        continue;
      }
      else
      {
        indices_size = scissor_rect.next_indices_idx - indices_start;
        _ctx.draw(indices_start, indices_size);
        indices_start = scissor_rect.next_indices_idx;
        scissor_rect  = data.pop_scissor_rect();
        indices_size  = scissor_rect.next_indices_idx - indices_start;
        _ctx.set_scissor_rect(scissor_rect.rect);
        goto label_draw_call_again;
      }
    }
    else if (draw_data.pipe_type == PipelineType::image)
    {
      auto& pipe = _pipes.at(PipelineType::image);

      _ctx.set_pipe(pipe.pipe_state.Get());
      _ctx.set_graphics_root_signature(pipe.root_signature);
      _ctx.set_primitive_topology(pipe.primive_topology);
      _ctx.set_graphics_descriptor(pipe.root_param_idx("image"), draw_data.image_descriptor_gpu_handle);
      _ctx.set_graphics_constants(pipe.root_param_idx("constants"), Constants
      {
        .window_extent = frame.image.extent(),
        .window_pos    = data.resizing_window_pos,
        .image_alpha   = draw_data.image_alpha,
      });
      _ctx.set_scissor_rect(scissor_rect.rect);
      _ctx.draw(draw_data.indices_start, draw_data.indices_size);
      continue;
    }
    std::unreachable();
  }
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
    std::string_view                   shader,
    std::string_view                   vs,
    std::string_view                   ps,
    std::string_view                   include,
    ImageFormat                        rtv_format,
    bool                               use_blend,
    bool                               use_depth_test,
    std::optional<RootSignatureResult> res
  ) noexcept
{
  auto compile_result = g_compiler.compile(shader, vs, ps, include, res);
  
  if (auto res = g_pipe_sys.find_root_param(compile_result.root_params))
    root_signature = res;
  else
    root_signature = g_pipe_sys._root_signatures.emplace_back(compile_result.root_signature, compile_result.root_params).first.Get();
  _root_param_idxs = compile_result.root_param_indices;

  auto stream = CD3DX12_PIPELINE_STATE_STREAM{};
  
  auto render_target_formats = D3D12_RT_FORMAT_ARRAY{};
  render_target_formats.NumRenderTargets = 1;
  render_target_formats.RTFormats[0]     = dxgi_format(rtv_format);
  
  stream.pRootSignature        = root_signature;
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
  err_if(g_core.device()->CreatePipelineState(&pipeline_state_stream_desc, IID_PPV_ARGS(&pipe_state)),
          "failed to create pipeline state");
}

void PipelineSystem::Pipeline::init_compute(std::string_view shader, std::string_view cs, std::string_view include, std::optional<RootSignatureResult> res) noexcept
{
  auto compile_result = g_compiler.compile(shader, cs, include, res);
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

void PipelineSystem::Context::set_cmd(ID3D12GraphicsCommandList1* cmd) noexcept
{
  _cmd = cmd;

  _pipe_state                        = {};
  _graphics_root_signature           = {};
  _compute_root_signature            = {};
  _graphics_constants_root_param_idx = {};
  _compute_constants_root_param_idx  = {};
  _primitive_topology                = {};
  _scissor_rect                      = {};
  _graphics_descriptors.clear();
  _compute_descriptors.clear();
  _graphics_constants.clear();
  _compute_constants.clear();
}

void PipelineSystem::Context::set_pipe(ID3D12PipelineState* pipe_state) noexcept
{
  if (_pipe_state != pipe_state)
  {
    _pipe_state = pipe_state;
    _cmd->SetPipelineState(_pipe_state);
  }
}

void PipelineSystem::Context::set_graphics_root_signature(ID3D12RootSignature* root_signature) noexcept
{
  if (_graphics_root_signature != root_signature)
  {
    _graphics_root_signature = root_signature;
    _cmd->SetGraphicsRootSignature(_graphics_root_signature);
  }
}

void PipelineSystem::Context::set_compute_root_signature(ID3D12RootSignature* root_signature) noexcept
{
  if (_compute_root_signature != root_signature)
  {
    _compute_root_signature = root_signature;
    _cmd->SetComputeRootSignature(_compute_root_signature);
  }
}

void PipelineSystem::Context::set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY primitive_topology) noexcept
{
  if (_primitive_topology != primitive_topology)
  {
    _primitive_topology = primitive_topology;
    _cmd->IASetPrimitiveTopology(_primitive_topology);
  }
}

void PipelineSystem::Context::set_scissor_rect(RECT rect) noexcept
{
  if (!EqualRect(&rect, &_scissor_rect))
  {
    _scissor_rect = rect;
    _cmd->RSSetScissorRects(1, &_scissor_rect);
  }
}

void PipelineSystem::Context::draw(uint32_t start_idx, uint32_t size) const noexcept
{
  _cmd->DrawIndexedInstanced(size, 1, start_idx, 0, 0);
}

void PipelineSystem::Context::set_graphics_descriptor(uint32_t root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept
{
  if (!_graphics_descriptors.contains(root_param_idx))
  {
    _graphics_descriptors[root_param_idx] = handle;
    _cmd->SetGraphicsRootDescriptorTable(root_param_idx, handle);
  }
  else if (_graphics_descriptors.at(root_param_idx).ptr != handle.ptr)
  {
    _graphics_descriptors[root_param_idx] = handle;
    _cmd->SetGraphicsRootDescriptorTable(root_param_idx, handle);
  }
}

void PipelineSystem::Context::set_compute_descriptor(uint32_t root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept
{
  if (!_compute_descriptors.contains(root_param_idx))
  {
    _compute_descriptors[root_param_idx] = handle;
    _cmd->SetComputeRootDescriptorTable(root_param_idx, handle);
  }
  else if (_compute_descriptors.at(root_param_idx).ptr != handle.ptr)
  {
    _compute_descriptors[root_param_idx] = handle;
    _cmd->SetComputeRootDescriptorTable(root_param_idx, handle);
  }
}

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

}}
