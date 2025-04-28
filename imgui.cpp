//
// this file use to test imgui reconginization
//

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>

struct Vertex
{
  glm::vec2 pos;
  glm::vec2 tex;
  uint32_t  col;
};

inline uint32_t constexpr float_to_uint32_saturate(float num)
{
  return num * 255.f + 0.5f;
}

auto constexpr convert_vec4_to_uint32_t(glm::vec4 const& vec4)
{
  uint32_t result;
  result  = float_to_uint32_saturate(vec4.x) << 0;
  result |= float_to_uint32_saturate(vec4.y) << 8;
  result |= float_to_uint32_saturate(vec4.z) << 16;
  result |= float_to_uint32_saturate(vec4.w) << 24;
  return result;
}

class DrawList
{
  VkCommandBuffer       cmd = VK_NULL_HANDLE;
  std::vector<Vertex>   vertices;
  std::vector<uint16_t> indices;

public:
  auto clear()
  {
    vertices.clear();
    indices.clear();
  }

  auto primitive_reserve(uint32_t vertex_count, uint32_t index_count)
  {
    vertices.reserve(vertices.size() + vertex_count);
    indices.reserve(indices.size() + index_count);
  }

  auto add_rectangle_filled(glm::vec2 const& min, glm::vec3 const& max, uint32_t color, float rounding)
  {
    if (rounding < .5f)
    {
      primitive_reserve(6, 4);

    }
    else
    {

    }
  }
};