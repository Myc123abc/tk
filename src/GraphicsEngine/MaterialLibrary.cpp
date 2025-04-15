#include "MaterialLibrary.hpp"
#include "ErrorHandling.hpp"

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
  default:
    throw_if(true, "unsupported color");
  }
}

Mesh create_quard(Color color)
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

////////////////////////////////////////////////////////////////////////////////
//                           Material Library 
////////////////////////////////////////////////////////////////////////////////

void MaterialLibrary::generate_materials()
{
  // enumerate all materials
  auto shape_types = std::vector<ShapeType>
  {
    ShapeType::Quard,
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
    case ShapeType::Quard:
      for (auto color : colors)
        shape_meshs.emplace_back(create_quard(color));
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
