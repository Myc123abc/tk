#include "GraphicsEngine.hpp"
#include "gltf.hpp"

namespace tk { namespace graphics_engine {

void GraphicsEngine::load_gltf()
{
  auto data = graphics_engine::load_gltf(this, "asset/monkey.glb");
  // _destructors.push([&]
  // { 
    for (auto& d : data)
    {
      d->mesh_buffer.destroy(_vma_allocator);
    }
  // });
}

} }
