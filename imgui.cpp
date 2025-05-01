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
    auto idx = _indices.size();
    _indices.append_range(std::vector<uint16_t>
    {
      static_cast<uint16_t>(idx), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2),
      static_cast<uint16_t>(idx), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 3),
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

  inline constexpr auto normalize(glm::vec2& point)
  {
    if (auto d2 = point.x * point.x + point.y * point.y > 0.f)
      point /= std::sqrt(d2);
  }

  inline constexpr auto approximate_normalize(glm::vec2& point)
  {
    if (auto d2 = point.x * point.x + point.y * point.y > 0.000001f)
    {
      auto inverse_len2 = 1.f / d2;
      // INFO: max best value 500.0f. (see imgui #4053, #3366)
      static constexpr auto Max_Inverse_Length_Squared = 100.f; 
      if (inverse_len2 > Max_Inverse_Length_Squared)
        inverse_len2 = Max_Inverse_Length_Squared;
      point *= inverse_len2;
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

    // calculate normals (tangents) for each line segment
    auto count = line_count - 1;
    for (auto i = 0, j = 1; i < count; ++i, ++j)
    {
      normals[i] = { _path[j].y - _path[i].y, _path[i].x - _path[j].x };
      normalize(normals[i]);
    }
    if (is_closed)
    {
      normals[count] = { _path[0].y - _path[count].y, _path[count].x - _path[0].x };
      normalize(normals[count]);
    }

    auto half_draw_size = thickness / 2 + 1;

    // TODO: if line is not closed, the first and last points need to be generated differently as there are no normals to blend
    if (!is_closed)
    {

    }

    auto idx_beg = _indices.size();
    for (auto i = 0, j = 1; i < count; ++i, ++j)
    {
      // average normal
      auto average_normal = glm::vec2((normals[i].x + normals[j].x) / 2, (normals[i].y + normals[j].y) / 2);
      approximate_normalize(average_normal);
      average_normal *= half_draw_size;

      // add normals for the outer edges
      auto* outs = &buf[j * 2];
      outs[0] = _path[j] + average_normal;
      outs[1] = _path[j] - average_normal;

      // set indices
      auto idx_end = idx_beg + 2;
      _indices.append_range(std::vector<uint16_t>
      {
        static_cast<uint16_t>(idx_end)    , static_cast<uint16_t>(idx_beg)    , static_cast<uint16_t>(idx_beg + 1),
        static_cast<uint16_t>(idx_end + 1), static_cast<uint16_t>(idx_beg + 1), static_cast<uint16_t>(idx_end)    ,
      });
      idx_beg = idx_end;
    }
  }

  auto path_stroke(uint32_t color, float thickness)
  {
    add_ployline(color, thickness, true);
    _path.clear();
  }

  // FIXME: I not use +0.5f, I will try 1 pixels sized rectangle
  auto add_rectangle(glm::vec2 const& upper_left, glm::vec2 const& lower_right, uint32_t color, float rounding, float thinkness)
  {
    // use the center of pixel
    path_rectangle(upper_left + .5f, lower_right - .5f, color, rounding);
    path_stroke(color, thinkness);
  }
};