#include "tk/GraphicsEngine/Device.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/vk_extension.hpp"
#include "tk/util.hpp"

#include <cassert>

namespace tk { namespace graphics_engine {

void Device::destroy() noexcept
{
  assert(_device);
  vkDestroyDevice(_device, nullptr);
  _device                   = VK_NULL_HANDLE;
  _descriptor_buffer_info   = {};
}

auto Device::init(VkPhysicalDevice device, VkDeviceCreateInfo const& info) -> Device&
{
  assert(device);
  
  throw_if(vkCreateDevice(device, &info, nullptr, &_device) != VK_SUCCESS,
           "failed to create device");

  VkPhysicalDeviceProperties2 device_properties
  {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    .pNext = &_descriptor_buffer_info,
  };
  vkGetPhysicalDeviceProperties2(device, &device_properties);

  return *this;
}

auto Device::create_descriptor_layout(std::vector<DescriptorInfo2> const& infos) -> DescriptorLayout
{
  return DescriptorLayout(this, infos);
}

void Device::create_shaders(std::vector<ShaderCreateInfo> const& infos, bool link)
{
  auto shader_datas = std::vector<std::vector<uint32_t>>();
  auto create_infos = std::vector<VkShaderCreateInfoEXT>();
  auto shaders      = std::vector<VkShaderEXT>(infos.size());
  shader_datas.reserve(infos.size());
  create_infos.reserve(infos.size());

  for (auto i = 0; i < infos.size(); ++i)
  {
    auto& info = infos[i];    
    shader_datas.emplace_back(util::get_file_data(info.shader_name));
    create_infos.emplace_back(VkShaderCreateInfoEXT
    {
      .sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
      .flags                  = link ? static_cast<VkShaderCreateFlagsEXT>(VK_SHADER_CREATE_LINK_STAGE_BIT_EXT) : 0,
      .stage                  = info.stage,
      .nextStage              = static_cast<VkShaderStageFlags>(info.stage == VK_SHADER_STAGE_VERTEX_BIT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0),
      .codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT,
      .codeSize               = shader_datas.back().size() * sizeof(uint32_t),
      .pCode                  = shader_datas.back().data(),
      .pName                  = "main",
      .setLayoutCount         = static_cast<uint32_t>(info.descriptor_layouts.size()),
      .pSetLayouts            = info.descriptor_layouts.data(),
      .pushConstantRangeCount = static_cast<uint32_t>(info.push_constants.size()),
      .pPushConstantRanges    = info.push_constants.data(),
    });
  };

  throw_if(graphics_engine::vkCreateShadersEXT(_device, create_infos.size(), create_infos.data(), nullptr, shaders.data()) != VK_SUCCESS,
           "failed to create shaders");

  for (auto i = 0; i < infos.size(); ++i)
    infos[i].shader.set(_device, shaders[i]);
}

}}