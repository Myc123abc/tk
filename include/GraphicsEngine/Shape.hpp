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
  struct TransformMatrixs
  {
    glm::mat4 model;
  };

  // for easy, we only have 2D position and color
  // maybe expand to texture in future
  struct Vertex
  {
    glm::vec2 position;
    glm::vec3 color;
  };

  // mesh have vertices and indices
  // indices use uint8_t for minimize byte necessary
  // simply 2D shape not need so many indices
  struct Mesh
  {
    std::vector<Vertex>  vertices;
    std::vector<uint8_t> indices;
  };

  // simple color
  enum class Color
  {
    Red,
    Green,
    Blue,
  };

  /*
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
    default:
      throw_if(true, "unsupported color");
    }
  }

  /*
   * create a quard mesh
   * @param width
   * @param height
   * @param color
   * @return mesh
   */
  inline Mesh create_quard(uint32_t width, uint32_t height, Color color)
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

} }
