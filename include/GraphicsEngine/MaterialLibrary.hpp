//
// Material Library
//
// singleton class, manage materials
//
// TODO:
// 1. dynamic clear resources avoid taking up too much memory
// 2. currently, only generate static materials at initialization,
//    expand to dynamic load materials
//

#pragma once

#include <vector>
#include <map>

namespace tk { namespace graphics_engine {

  enum class ShapeType
  {
    Quard,
  };

  enum class Color
  {
    Red,
    Green,
    Blue,
    Yellow,
    OneDark,
  };

  struct Material
  {
    ShapeType          type;
    std::vector<Color> colors;
  };

  class MaterialLibrary
  {
  public:
    static auto create_mesh_buffer(class Command& cmd) -> class MeshBuffer;

  private:
    MaterialLibrary();
    ~MaterialLibrary() = default;

    MaterialLibrary(MaterialLibrary const&)            = delete;
    MaterialLibrary(MaterialLibrary&&)                 = delete;
    MaterialLibrary& operator=(MaterialLibrary const&) = delete;
    MaterialLibrary& operator=(MaterialLibrary&&)      = delete;

    static auto instance() -> MaterialLibrary const&
    {
      static MaterialLibrary instance;
      return instance;
    }

  private:
    static std::map<ShapeType, std::vector<Color>> _shape_infos;
  };

}}
