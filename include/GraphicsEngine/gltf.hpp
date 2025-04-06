//
// use fastgltf to load gltf
//

#pragma once

#include "Buffer.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <memory>

namespace tk { namespace graphics_engine {

  struct GeometrySurface
  {
    uint32_t start_index = 0;
    uint32_t count       = 0;
  };

  struct MeshAsset
  {
    std::string                  name;
    std::vector<GeometrySurface> surfaces;
    MeshBuffer                   mesh_buffer; 
  };

  auto load_gltf(class GraphicsEngine* engine, std::filesystem::path file_path) -> std::vector<std::shared_ptr<MeshAsset>>;

} }
