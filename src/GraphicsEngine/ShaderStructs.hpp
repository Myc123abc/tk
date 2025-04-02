#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>


namespace tk { namespace graphics_engine {

  struct PushContant
  {
    glm::vec4 data1;
    glm::vec4 data2;
  };

  struct UniformBufferObject
  {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };
  
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 color;
  
    static constexpr auto get_attribute_descriptions() -> std::vector<VkVertexInputAttributeDescription>
    {
      return
      {
        {
          .location = 0,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(Vertex, position),
        },
        {
          .location = 1,
          .binding  = 0,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(Vertex, color),
        },
      };
    }
  
    static constexpr auto get_binding_description()
    {
      return VkVertexInputBindingDescription
      {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      };
    }
  
  };

} }
