#include "tk/GraphicsEngine/MaterialLibrary.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/DestructorStack.hpp"
#include "tk/GraphicsEngine/CommandPool.hpp"

#include <numbers>

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                           Mesh Generate Functions
////////////////////////////////////////////////////////////////////////////////

Mesh create_quard()
{
  return Mesh
  { 
    {
      { { -1.f, -1.f } },
      { {  1.f, -1.f } },
      { {  1.f,  1.f } },
      { { -1.f,  1.f } },
    },
    {
      0, 1, 2,
      0, 2, 3,
    },
  };
};

Mesh create_circle()
{
  auto vertices = std::vector<Vertex>();
  auto indices  = std::vector<uint16_t>();

  constexpr auto radius   = 1.f;
  constexpr auto segments = 16;

  vertices.reserve(segments + 1);
  indices.reserve(segments * 3);

  // set center vertex
  vertices.emplace_back(Vertex{{ 0.f, 0.f }});
  uint16_t center_index = 0;

  // set other vertices
  for (uint32_t i = 0; i < segments; ++i)
  {
    float angle = 2.f * std::numbers::pi * (float)i / segments;
    float x     = radius * std::cos(angle);
    float y     = radius * std::sin(angle);
    vertices.emplace_back(Vertex{{ x, y }});
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

  // save all material combination
  for (auto shape_type : shape_types)
    _materials.emplace_back(shape_type);
}

auto MaterialLibrary::get_meshs() -> std::vector<Mesh>
{
  if (_materials.empty())
    generate_materials();

  // get meshs
  auto shape_meshs = std::vector<Mesh>();
  for (auto const& material : _materials)  
  {
    switch (material.type)
    {
    case ShapeType::Unknow:
      throw_if(true, "unknow shape type");

    case ShapeType::Quard:
      shape_meshs.emplace_back(create_quard());
      break;

    case ShapeType::Circle:
      shape_meshs.emplace_back(create_circle());
      break;
    }
  }

  return std::move(shape_meshs);
}

void MaterialLibrary::build_mesh_infos(std::span<MeshInfo> mesh_infos)
{
  uint32_t i = 0;
  for (auto const& material : _materials)
  {
    _mesh_infos[material.type] = mesh_infos[i];
    ++i;
  }
}

}}
