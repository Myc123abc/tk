#pragma once

#include "../../../util/hash.hpp"
#include "../../../util/singleton.hpp"
#include "util/base.hpp"

#include <directx/d3dx12.h>
#include <dxcapi.h>
#include <d3d12shader.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <optional>

namespace tk::renderer {

struct DescriptorInfo
{
  enum class Type
  {
    constants,
    byte_buffer,
    structured_buffer,
    texture,
    rw_texture,
    textures
  };

  Type        type{};
  std::string name;
  uint        bind_point{};
  uint        space{};
  bool        is_volatile{};
  uint        constants_size{};
};

struct RootSignatureResult
{
  Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
  std::unordered_map<std::string, uint>       root_param_indices;
  std::vector<CD3DX12_ROOT_PARAMETER1>        root_params;
};

auto generate_root_signature(std::vector<DescriptorInfo> const& desc_infos, bool is_graphics = false, bool use_sampler = false) noexcept -> RootSignatureResult;

Singleton(Compiler, g_compiler,
public:
  void init() noexcept;

  struct CompileResult
  {
    D3D12_SHADER_BYTECODE                       vs;
    D3D12_SHADER_BYTECODE                       ps;
    D3D12_SHADER_BYTECODE                       cs;
    D3D12_INPUT_LAYOUT_DESC                     input_layout_desc;
    std::unordered_map<std::string, uint>       root_param_indices;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    std::vector<CD3DX12_ROOT_PARAMETER1>        root_params;

  private:
    friend class Compiler;

    auto get_shader_reflection(IDxcResult* result) noexcept -> Microsoft::WRL::ComPtr<ID3D12ShaderReflection>;

    void get_vertex_input_layout(ID3D12ShaderReflection* shader_reflection) noexcept;

    void get_root_parameters(ID3D12ShaderReflection* shader_reflection, bool is_compute_shader, std::unordered_set<std::string_view> const& volatile_descs) noexcept;
 
  private:
    Microsoft::WRL::ComPtr<IDxcBlob>      _vs_cso;
    Microsoft::WRL::ComPtr<IDxcBlob>      _ps_cso;
    Microsoft::WRL::ComPtr<IDxcBlob>      _cs_cso;
    std::vector<D3D12_INPUT_ELEMENT_DESC> _input_element_descs;
    std::vector<std::string>              _input_param_names;
    std::queue<CD3DX12_DESCRIPTOR_RANGE1> _ranges;
    bool                                  _has_sampler{};
    
    struct ResourceKey
    {
      D3D_SHADER_INPUT_TYPE type;
      uint                  bind_point{};
      uint                  space{};

      bool operator==(ResourceKey const& x) const noexcept
      {
        return type       == x.type       &&
               bind_point == x.bind_point &&
               space      == x.space;
      }
    };
    struct ResourceKeyHash
    {
      auto operator()(ResourceKey const& key) const noexcept
      {
        return generic_hash(key.type, key.bind_point, key.space);
      }
    };
    std::unordered_set<ResourceKey, ResourceKeyHash> _resource_keys;
  };

  auto compile(
    std::string_view                            shader,
    std::string_view                            vertex_shader_entry_point,
    std::string_view                            pixel_shader_entry_point,
    std::vector<std::string_view> const&        includes       = {},
    std::optional<RootSignatureResult>          res            = {},
    std::unordered_set<std::string_view> const& volatile_descs = {}) noexcept -> CompileResult;

  auto compile(
    std::string_view                            shader,
    std::string_view                            compute_shader_entry_point,
    std::vector<std::string_view> const&        includes       = {},
    std::optional<RootSignatureResult>          res            = {},
    std::unordered_set<std::string_view> const& volatile_descs = {}) noexcept -> CompileResult;

private:
  auto compile(std::string_view shader_path, std::vector<std::string_view> const& includes, std::wstring_view profile, std::string_view entry_point) noexcept -> std::pair<Microsoft::WRL::ComPtr<IDxcResult>, Microsoft::WRL::ComPtr<IDxcBlob>>;

private:
  Microsoft::WRL::ComPtr<IDxcCompiler3>      _compiler;
  Microsoft::WRL::ComPtr<IDxcUtils>          _utils;
  Microsoft::WRL::ComPtr<IDxcIncludeHandler> _include_handler;
)

}
