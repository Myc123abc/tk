#pragma once

#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"

#include <vulkan/vulkan.h>

#include <vector>

// TODO:
// should have same type descriptor binding
// such as smaa

namespace tk { namespace graphics_engine {

  // TODO:
  //  1. only process image now
  //  2. binding and type cannot duplicate
  //  3. assume only single variable descriptors can be used now
  //  4. assume all use combined image now
  struct DescriptorInfo
  {
    uint32_t            binding{};
    VkDescriptorType    type{};
    VkShaderStageFlags  stages{};
    std::vector<Image*> images{}; // when image size is multiple, it's should use variable-sized descriptors in shader
    VkSampler           sampler{};
  };

  void bind_descriptor_buffer(Command const& cmd, Buffer const& buffer);

  class DescriptorLayout
  {
  public:
    DescriptorLayout()  = default;
    ~DescriptorLayout() = default;

    // FIXME: discard, should integrate DescriptorLayout to other class
    operator VkDescriptorSetLayout() const noexcept { return _layout; }
    auto get_address() { return &_layout; }

    void destroy() noexcept;

    auto size() const noexcept { return _size; }

    void upload(Buffer& buffer, std::string_view tag);

    auto update() -> uint32_t;

    void bind(Command const& cmd);

    void set(VkPipelineLayout pipeline_layout, VkPipelineBindPoint bind_point) noexcept
    {
      _pipeline_layout = pipeline_layout;
      _bind_point      = bind_point;
    }

  private:
    friend class Device;

    DescriptorLayout(class Device* device, std::vector<DescriptorInfo> const& layouts);

    void create_descriptor_pool(std::vector<DescriptorInfo> const& layouts);
    void create_descriptor_set_layout(std::vector<DescriptorInfo> const& descriptor_infos); // TODO: need to be distinguished from using descriptor buffer features
    void allocate_descriptor_sets(uint32_t variable_descriptor_count);
    void update_descriptor_sets(std::vector<DescriptorInfo> const& descriptor_infos);

    auto get_variable_descriptor_count(std::vector<DescriptorInfo> const& descriptor_infos) -> uint32_t;

    void create_descrptor_set_layout(std::vector<VkDescriptorSetLayoutBinding> const& bindings);
    void create_descriptor_pool(std::unordered_map<int, uint32_t> const& types);
    void update_descriptor_sets();
    auto update_descriptor_buffer() -> uint32_t;
    
  private:
    class Device*                _device{};
    VkDescriptorSetLayout        _layout{};
    VkDeviceSize                 _size{};
    std::vector<DescriptorInfo>  _descriptors;
    VkPipelineLayout             _pipeline_layout{};
    VkPipelineBindPoint          _bind_point{};
    std::string                  _tag{};
    Buffer*                      _buffer{};
    char*                        _ptr{};

    VkDescriptorPool             _pool{};
    VkDescriptorSet              _set{};
  };

}}