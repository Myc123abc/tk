#include "gltf.hpp"
#include "ErrorHandling.hpp"
#include "GraphicsEngine.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

namespace tk { namespace graphics_engine {

auto load_gltf(class GraphicsEngine* engine, std::filesystem::path file_path) -> std::vector<std::shared_ptr<MeshAsset>>
{
  // read data from glft
  auto data = fastgltf::GltfDataBuffer().FromPath(file_path);
  auto load = fastgltf::Parser().loadGltfBinary(data.get(), file_path.parent_path(), fastgltf::Options::LoadExternalBuffers);
  throw_if(load.error() != fastgltf::Error::None, "failed to load gltf");
  auto asset = std::move(load.get());

  // transform data to vector<MeshAsset>
  auto meshs    = std::vector<std::shared_ptr<MeshAsset>>();
  auto vertices = std::vector<Vertex>();
  auto indices  = std::vector<uint32_t>();
  for (auto& mesh : asset.meshes)
  {
    MeshAsset mesh_asset;
    mesh_asset.name = mesh.name;

    vertices.clear();
    indices.clear();

    for (auto&& p : mesh.primitives)
    {
      GeometrySurface surface;
      surface.start_index = indices.size();
      surface.count       = asset.accessors[p.indicesAccessor.value()].count;

      auto init_vertex = vertices.size();
      // load indices
      {
        auto& index_accessor = asset.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + index_accessor.count);
        fastgltf::iterateAccessor<std::uint32_t>(asset, index_accessor,
        [&](std::uint32_t idx)
        {
          indices.push_back(idx + init_vertex);
        });
      }

      // load vertices
      {
        auto& pos_accessor = asset.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + pos_accessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, pos_accessor,
        [&](glm::vec3 v, size_t idx)
        {
          Vertex vtx = {};
          vtx.pos    = v;
          vtx.color  = glm::vec4(1.f);
          vtx.normal = { 1, 0, 0 };
          vertices[init_vertex + idx] = vtx;
        });
      }

      // load normal
      {
        auto normals = p.findAttribute("NORMAL");
        if (normals != p.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, asset.accessors[normals->accessorIndex],
          [&](glm::vec3 v, size_t idx)
          {
            vertices[init_vertex + idx].normal = v;
          });
        }
      }

      // load uv
      {
        auto uvs = p.findAttribute("TEXCOORD_0");
        if (uvs != p.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec2>(asset, asset.accessors[uvs->accessorIndex],
          [&](glm::vec2 v, size_t idx)
          {
            vertices[init_vertex + idx].uv_x = v.x;
            vertices[init_vertex + idx].uv_y = v.y;
          });
        }
      }

      // load color
      {
        auto colors = p.findAttribute("COLOR_0");
        if (colors != p.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec4>(asset, asset.accessors[colors->accessorIndex],
          [&](glm::vec4 v, size_t idx)
          {
            vertices[init_vertex + idx].color = v;
          });
        }
      }

      mesh_asset.surfaces.push_back(surface);
    }

    for (auto& vtx : vertices)
    {
      vtx.color = glm::vec4(vtx.normal, 1.f);
    }

    mesh_asset.mesh_buffer = engine->create_mesh_buffer(vertices, indices);
    meshs.emplace_back(std::make_shared<MeshAsset>(std::move(mesh_asset)));
  }

  return meshs;
}

} }
