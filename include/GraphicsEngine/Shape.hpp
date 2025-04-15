//
// Shape
//
// basic shape to generic 2D graphics mesh
//

#pragma once

#include "ErrorHandling.hpp"

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace tk { namespace graphics_engine {

  // only transform object from local to world
  struct PushConstant 
  {
    glm::mat4       model;
    VkDeviceAddress vertices = {};
  };

  // for easy, we only have 2D position and color
  // maybe expand to texture in future
  struct Vertex
  {
    alignas(16) glm::vec2 position;
    alignas(16) glm::vec3 color;
  };

  // mesh have vertices and indices
  // indices use uint8_t for minimize byte necessary
  // simply 2D shape not need so many indices
  struct Mesh
  {
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;
  };

  enum class Color
  {
    Red,
    Green,
    Blue,
    Yellow,
    OneDark,
  };

  /**
   * transform color to vec3 for generate vertex
   * @param color
   * @return vec3
   * @throw std::runtime_error unsupported color
   */
  inline auto to_vec3(Color color) -> glm::vec3
  {
    switch (color)
    {
    case Color::Red:
      return { 1.f, 0.f, 0.f };
    case Color::Green:
      return { 0.f, 1.f, 0.f };
    case Color::Blue:
      return { 0.f, 0.f, 1.f };
    case Color::Yellow:
      return { 1.f, 1.f, 0.f };
    case Color::OneDark:
      return { (float)40/255, (float)44/255, (float)52/255,};
    default:
      throw_if(true, "unsupported color");
    }
  }

  /**
   * create a standard quard mesh which width and height is 2 and position in center
   * @param color
   * @return mesh
   */
  inline Mesh create_quard(Color color)
  {
    return Mesh
    { 
      {
        { { -1.f, -1.f }, to_vec3(color) },
        { {  1.f, -1.f }, to_vec3(color) },
        { {  1.f,  1.f }, to_vec3(color) },
        { { -1.f,  1.f }, to_vec3(color) },
      },
      {
        0, 1, 2,
        0, 2, 3,
      },
    };
  };

}}
