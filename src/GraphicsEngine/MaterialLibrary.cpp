#include "tk/GraphicsEngine/MaterialLibrary.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/DestructorStack.hpp"
#include "tk/GraphicsEngine/CommandPool.hpp"

#include <numbers>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                           Mesh Generate Functions
////////////////////////////////////////////////////////////////////////////////

auto to_vec3(Color color) -> glm::vec3
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
  case Color::Unknow:
    throw_if(true, "unknow color");
    return {};
  };
  return {};
}

Mesh create_quard(Color color)
{
  auto col = to_vec3(color);
  return Mesh
  { 
    {
      { { -1.f, -1.f }, col },
      { {  1.f, -1.f }, col },
      { {  1.f,  1.f }, col },
      { { -1.f,  1.f }, col },
    },
    {
      0, 1, 2,
      0, 2, 3,
    },
  };
};

Mesh create_circle(Color color)
{
  auto vertices = std::vector<Vertex>();
  auto indices  = std::vector<uint16_t>();
  auto col      = to_vec3(color);

  constexpr auto radius   = 1.f;
  constexpr auto segments = 16;

  vertices.reserve(segments + 1);
  indices.reserve(segments * 3);

  // set center vertex
  vertices.emplace_back(Vertex{{ 0.f, 0.f }, col});
  uint16_t center_index = 0;

  // set other vertices
  for (uint32_t i = 0; i < segments; ++i)
  {
    float angle = 2.f * std::numbers::pi * (float)i / segments;
    float x     = radius * std::cos(angle);
    float y     = radius * std::sin(angle);
    vertices.emplace_back(Vertex{{ x, y }, col});
  }

  // set indices
  for (uint16_t i = 1; i < segments; ++i)
  {
    indices.push_back(center_index);
    indices.push_back(i);
    indices.push_back(i + 1);
  }
  indices.push_back(center_index);
  indices.push_back(segments);
  indices.push_back(1);

  return { vertices, indices };
}

////////////////////////////////////////////////////////////////////////////////
//                           Material Library 
////////////////////////////////////////////////////////////////////////////////

void MaterialLibrary::init(MemoryAllocator& mem_alloc, Command& cmd, DestructorStack& destructor)
{
  _mem_alloc = &mem_alloc;

  // get shape meshs
  auto meshs = MaterialLibrary::get_meshs();

  // create mesh buffer
  auto mesh_infos = std::vector<MeshInfo>();
  _mesh_buffer = mem_alloc.create_mesh_buffer(cmd, meshs, destructor, mesh_infos);

  // build mesh info with shape type
  MaterialLibrary::build_mesh_infos(mesh_infos);
}

void MaterialLibrary::destroy()
{
  assert(_mem_alloc);
  _mem_alloc->destroy_mesh_buffer(_mesh_buffer);
  _mem_alloc = nullptr;
  _materials.clear();
  _mesh_infos.clear(); 
}

void MaterialLibrary::generate_materials()
{
  // enumerate all materials
  auto shape_types = std::vector<ShapeType>
  {
    ShapeType::Quard,
    ShapeType::Circle,
  };

  auto colors = std::vector<Color>
  {
    Color::Red,
    Color::Green,
    Color::Blue,
    Color::Yellow,
    Color::OneDark,
  };

  // save all material combination
  for (auto shape_type : shape_types)
    _materials.emplace_back(shape_type, colors);
}

auto MaterialLibrary::get_meshs() -> std::vector<Mesh>
{
  if (_materials.empty())
    generate_materials();

  // get meshs
  auto shape_meshs = std::vector<Mesh>();
  for (auto const& [type, colors] : _materials)  
  {
    switch (type)
    {
    case ShapeType::Unknow:
      throw_if(true, "unknow shape type");

    case ShapeType::Quard:
      for (auto color : colors)
        shape_meshs.emplace_back(create_quard(color));
      break;

    case ShapeType::Circle:
      for (auto color : colors)
        shape_meshs.emplace_back(create_circle(color));
      break;
    }
  }

  return std::move(shape_meshs);
}

void MaterialLibrary::build_mesh_infos(std::span<MeshInfo> mesh_infos)
{
  uint32_t i = 0;
  for (auto const& [type, colors] : _materials)
  {
    for (auto color : colors)
    {
      _mesh_infos[type][color] = mesh_infos[i];
      ++i;
    }
  }
}

}}
