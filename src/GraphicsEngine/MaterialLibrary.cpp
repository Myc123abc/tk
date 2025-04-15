#include "MaterialLibrary.hpp"
#include "MemoryAllocator.hpp"
#include "CommandPool.hpp"

namespace tk { namespace graphics_engine {

MaterialLibrary::MaterialLibrary()
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
    for (auto color : colors)
      _shape_infos.emplace(shape_types, colors);
}

auto MaterialLibrary::create_mesh_buffer(Command& cmd) -> MeshBuffer
{
  MeshBuffer buffer;

  // get shape meshs
  std::vector<Mesh> ss;
  for (auto [shape_type, colors] : _shape_infos)  
  {
    switch (shape_type)
    {
    case ShapeType::Quard:
      for (auto color : colors)
      {
        
      }
      break;
    }
  }


  // for (auto const& info : canvas.shape_infos)
  // {
  //   switch (info->type) 
  //   {
  //   case ShapeType::Quard:
  //     shape_matrixs.emplace_back(info->type, info->color, get_quard_matrix(dynamic_cast<QuardInfo const&>(*info), *canvas.window, canvas.x, canvas.y));
  //     break;
  //   }
  // auto shape_meshs = _painter.get_shape_meshs();
  // auto meshs       = std::vector<Mesh>();
  // auto mesh_infos  = std::vector<MeshInfo>();
  // auto destructor  = DestructorStack();
  // meshs.reserve(shape_meshs.size());
  // for (auto& [type, color_mesh] : shape_meshs)
  //   for (auto& [color, mesh] : color_mesh)
  //     meshs.emplace_back(mesh);
  //
  // // create mesh buffer
  // auto cmd     = _command_pool.create_command().begin();
  // _mesh_buffer = _mem_alloc.create_mesh_buffer(cmd, meshs, destructor, mesh_infos);
  // cmd.end().submit_wait_free(_command_pool, _graphics_queue);
  //
  // // destroy stage buffer
  // destructor.clear();
  //
  // // add mesh buffer destructor
  // _destructors.push([&] { _mem_alloc.destroy_mesh_buffer(_mesh_buffer); });
  //
  // // get mesh info with shape type
  // uint32_t i = 0;
  // for (auto& [type, color_mesh] : shape_meshs)
  // {
  //   for (auto& [color, mesh] : color_mesh)
  //   {
  //     // TODO: these should be got by painter
  //     _shape_mesh_infos[type][color] = mesh_infos[i];
  //     ++i;
  //   }
  // }
  
  return buffer;
}

}}
