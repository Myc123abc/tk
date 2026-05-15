#include "context.hpp"

namespace tk::renderer {

void Context::set_cmd(ID3D12GraphicsCommandList1* cmd) noexcept
{
  _cmd = cmd;

  _pipe_state                        = {};
  _graphics_root_signature           = {};
  _compute_root_signature            = {};
  _graphics_constants_root_param_idx = {};
  _compute_constants_root_param_idx  = {};
  _primitive_topology                = {};
  _viewport                          = {};
  _scissor_rect                      = {};
  _stencil_value                     = {};
  _render_target                     = {};
  _depth_stencil                     = {};
  _graphics_descriptors.clear();
  _compute_descriptors.clear();
  _graphics_constants.clear();
  _compute_constants.clear();
}

void Context::set_pipe(ID3D12PipelineState* pipe_state) noexcept
{
  if (_pipe_state != pipe_state)
  {
    _pipe_state = pipe_state;
    _cmd->SetPipelineState(_pipe_state);
  }
}

void Context::set_graphics_root_signature(ID3D12RootSignature* root_signature) noexcept
{
  if (_graphics_root_signature != root_signature)
  {
    _graphics_root_signature = root_signature;
    _cmd->SetGraphicsRootSignature(_graphics_root_signature);
  }
}

void Context::set_compute_root_signature(ID3D12RootSignature* root_signature) noexcept
{
  if (_compute_root_signature != root_signature)
  {
    _compute_root_signature = root_signature;
    _cmd->SetComputeRootSignature(_compute_root_signature);
  }
}

void Context::set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY primitive_topology) noexcept
{
  if (_primitive_topology != primitive_topology)
  {
    _primitive_topology = primitive_topology;
    _cmd->IASetPrimitiveTopology(_primitive_topology);
  }
}

void Context::set_viewport(Rect rect) noexcept
{
  if (_viewport.replace(rect))
  {
    auto vp = CD3DX12_VIEWPORT{ rect.left, rect.top, rect.width(), rect.height() };
    _cmd->RSSetViewports(1, &vp);
  }
}

void Context::set_scissor_rect(Rect rect) noexcept
{
  if (_scissor_rect.replace(rect))
  {
    auto rc = rect.to_RECT();
    _cmd->RSSetScissorRects(1, &rc);
  }
}

void Context::set_stencil_value(uint value) noexcept
{
  if (!_stencil_value || _stencil_value.value() != value)
  {
    _stencil_value = value;
    _cmd->OMSetStencilRef(value);
  }
}

void Context::draw(uint count) const noexcept
{
  _cmd->DrawInstanced(3 * count, 1, 0, 0);
}

void Context::draw(uint start_idx, uint size) const noexcept
{
  _cmd->DrawIndexedInstanced(size, 1, start_idx, 0, 0);
}

void Context::dispatch(uint x, uint y, uint z) const noexcept
{
  _cmd->Dispatch(x, y, z);
}

void Context::set_graphics_descriptor(uint root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept
{
  if (!_graphics_descriptors.contains(root_param_idx))
  {
    _graphics_descriptors[root_param_idx] = handle;
    _cmd->SetGraphicsRootDescriptorTable(root_param_idx, handle);
  }
  else if (_graphics_descriptors[root_param_idx].ptr != handle.ptr)
  {
    _graphics_descriptors[root_param_idx] = handle;
    _cmd->SetGraphicsRootDescriptorTable(root_param_idx, handle);
  }
}

void Context::set_compute_descriptor(uint root_param_idx, D3D12_GPU_DESCRIPTOR_HANDLE handle) noexcept
{
  if (!_compute_descriptors.contains(root_param_idx))
  {
    _compute_descriptors[root_param_idx] = handle;
    _cmd->SetComputeRootDescriptorTable(root_param_idx, handle);
  }
  else if (_compute_descriptors[root_param_idx].ptr != handle.ptr)
  {
    _compute_descriptors[root_param_idx] = handle;
    _cmd->SetComputeRootDescriptorTable(root_param_idx, handle);
  }
}

void Context::set_render_target(Image* render_tareget_image, Image* depth_stencil_image) noexcept
{
  assert(render_tareget_image);
  render_tareget_image->set_state(_cmd, ImageState::render_target);
  auto rtv = render_tareget_image->rtv().cpu_handle();
  auto dsv = depth_stencil_image ? depth_stencil_image->dsv().cpu_handle() : D3D12_CPU_DESCRIPTOR_HANDLE{};
  assert(rtv.ptr);
  if (rtv.ptr != _render_target.ptr || dsv.ptr != _depth_stencil.ptr)
  {
    if (dsv.ptr)
      _cmd->OMSetRenderTargets(1, &rtv, false, &dsv);
    else
      _cmd->OMSetRenderTargets(1, &rtv, false, nullptr);
    _render_target = rtv;
    _depth_stencil = dsv;
  }
}

}
