#include "GraphicsEngine.hpp"
#include "gltf.hpp"

namespace tk { namespace graphics_engine {

void GraphicsEngine::load_gltf()
{
  _meshs = graphics_engine::load_gltf(this, "asset/monkey.glb");
  _destructors.push([&]
  { 
    for (auto& d : _meshs)
    {
      d->mesh_buffer.destroy(_vma_allocator);
    }
  });
}

} }
