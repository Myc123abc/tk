//
// this file use to test imgui reconginization
//

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>
#include <cassert>

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
  VkCommandBuffer        _cmd = VK_NULL_HANDLE;
  std::vector<Vertex>    _vertices;
  std::vector<uint16_t>  _indices;
  std::vector<glm::vec2> _path;

public:
  auto clear()
  {
    _vertices.clear();
    _indices.clear();
    _path.clear();
  }

  auto primitive_reserve(uint32_t vertex_count, uint32_t index_count)
  {
    _vertices.reserve(_vertices.size() + vertex_count);
    _indices.reserve(_indices.size() + index_count);
  }

  auto primitive_rectangle(glm::vec2 const& upper_left, glm::vec2 const& lower_right, uint32_t color)
  {
    _vertices.append_range(std::vector<Vertex>
    {
      {   upper_left,                    {}, color },
      { { lower_right.x, upper_left.y }, {}, color },
      {   lower_right,                   {}, color },
      { { upper_left.x, lower_right.y }, {}, color },
    });
    _indices.append_range(std::vector<uint16_t>
    {
      0, 1, 2,
      0, 2, 3,
    });
  }
  
  auto add_rectangle_filled(glm::vec2 const& upper_left, glm::vec3 const& lower_right, uint32_t color, float rounding)
  {
    if (rounding < .5f)
    {
      primitive_rectangle(upper_left, lower_right, color);
    }
    else
    {
      // TODO: rounding rectangle filled
    }
  }

  auto path_to_line(glm::vec2 const& point)
  {
    _path.emplace_back(point);
  }

  auto path_rectangle(glm::vec2 const& upper_left, glm::vec2 const& lower_right, uint32_t color, float rounding)
  {
    if (rounding >= .5f)
    {
      // TODO: rounding rectangle
    }
    
    if (rounding < .5f)
    {
      path_to_line(upper_left);
      path_to_line({lower_right.x, upper_left.x});
      path_to_line(lower_right);
      path_to_line({upper_left.x, lower_right.y});
    }
    else 
    {
      // TODO: rounding rectangle
    }
  }

  // TODO: Thickness anti-aliased lines cap are missing their AA fringe
  // TODO: how to draw thickness > 1.f
  auto add_ployline(uint32_t color, float thickness, bool is_closed)
  {
    assert(_path.size() > 1 && thickness > 1.f);
    // FIXME: should I make alpha == 0 be transparent
    assert(color & 0xFF000000);

    auto point_count = _path.size();
    auto line_count  = is_closed ? point_count : point_count - 1;
    auto color_transparent = color & ~0xFF000000;
    
    // TODO: _FringeScale; [Internal] anti-alias fringe is scaled by this value, this helps to keep things sharp while zooming at vertex buffer content
    // in imgui_draw.cpp:AddPolyline:AA_SIZE = _FringeScale;

    primitive_reserve(point_count * 2, line_count * 6);

    std::vector<glm::vec2> buf(point_count * 3);
    glm::vec2* normals = buf.data();
    glm::vec2* points  = normals + point_count;
  }

  auto path_stroke(uint32_t color, float thickness)
  {
    add_ployline(color, thickness, true);
    _path.clear();
  }

  // FIXME: I not use +0.5f, I will try 1 pixels sized rectangle
  auto add_rectangle(glm::vec2 const& upper_left, glm::vec3 const& lower_right, uint32_t color, float rounding, float thinkness)
  {
    path_rectangle(upper_left, lower_right, color, rounding);
    path_stroke(color, thinkness);
  }
};