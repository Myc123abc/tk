#pragma once

#include "resource/image.hpp"

#include <string>
#include <initializer_list>
#include <unordered_map>

namespace tk { namespace renderer {

class Pipeline
{
public:
  Pipeline()                           = default;
  ~Pipeline()                          = default;
  Pipeline(Pipeline const&)            = delete;
  Pipeline(Pipeline&&)                 = delete;
  Pipeline& operator=(Pipeline const&) = delete;
  Pipeline& operator=(Pipeline&&)      = delete;
  
  void init_graphics(
    std::string shader,
    std::string vs,
    std::string ps,
    std::string include,
    ImageFormat rtv_format,
    bool        use_blend      = false,
    bool        use_depth_test = false
  ) noexcept;

  void init_compute(std::string shader, std::string cs, std::string include = {}) noexcept;

  void bind(ID3D12GraphicsCommandList1* cmd) const noexcept;

  void set_descriptors(ID3D12GraphicsCommandList1* cmd, std::initializer_list<std::pair<std::string_view, D3D12_GPU_DESCRIPTOR_HANDLE>> handles) const noexcept;
  void set_descriptor(ID3D12GraphicsCommandList1* cmd, std::string_view name, D3D12_GPU_DESCRIPTOR_HANDLE handle) const noexcept;

  template <typename ConstantsType>
  void set_constants(ID3D12GraphicsCommandList1* cmd, std::string_view constants_name, ConstantsType const& constants) const noexcept
  {
    _is_graphics_pipeline
      ? cmd->SetGraphicsRoot32BitConstants(_resource_indices.at(constants_name.data()), sizeof(constants) / 4, &constants, 0)
      : cmd->SetComputeRoot32BitConstants(_resource_indices.at(constants_name.data()), sizeof(constants) / 4, &constants, 0);
  }

private:
  Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipeline_state;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> _root_signature;
  std::unordered_map<std::string, uint32_t>   _resource_indices;
  bool                                        _is_graphics_pipeline{};
};

}}
