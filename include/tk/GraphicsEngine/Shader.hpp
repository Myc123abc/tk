//
// Shader something
//
// pushconstant, vertex
//

#pragma once

#include "tk/type.hpp"

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  struct PushConstant_AAA_preprocessing
  {
    VkDeviceAddress buffer = {};
    glm::vec2       window_extent;
    glm::vec2       display_pos;
  };

  struct PushConstant_AAA
  {
    VkDeviceAddress buffer             = {};
    uint32_t        shape_infos_offset = {};
    uint32_t        shape_num          = {};
  };

  // TODO: currently, not concern about color and uv of per point
  struct ShapeInfo
  {
    std::vector<glm::vec2> points;
    uint32_t               color;
    tk::type::shape        type;
  };

  struct alignas(8) BufferShapeInfo
  {
    glm::vec4 color     = {};
    uint32_t  type      = {};
    uint32_t  offset    = {};
    uint32_t  point_num = {};
    uint32_t  pad0      = {};
  };

  //
  // smaa
  // 
  struct PushConstant_SMAA
  {
    // TODO: maybe i can only transform vec2 and calculate tr_metrics in shader
    glm::vec4 smaa_rt_metrics;
  };

  class Shader final
  {
  public:
    Shader()  = default;
    ~Shader() = default;

    operator VkShaderEXT() const noexcept { return _shader; }

    void destroy();

  private:
    friend class Device;

    void set(VkDevice device, VkShaderEXT shader) noexcept;

  private:
    VkDevice    _device{};
    VkShaderEXT _shader{};
  };

}}