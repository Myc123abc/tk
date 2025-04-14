//
// Painter
//
// a painter can draw on canvas.
// you should create your canvas, then draw.
//
// HACK:
// 1. use double key(type, color) to store meshs
//    same shape type have multiple color, shape vertices will be repeately
//    this is memory waste
// 2. present with draw desquence, not have depth test
//

#pragma once

#include "Shape.hpp"

#include <vector>
#include <string_view>
#include <map>
#include <memory>

namespace tk { namespace graphics_engine {

  enum class ShapeType
  {
    Quard,
  };

  class Painter
  {
  public:
    Painter()  = default;
    ~Painter() = default;

    Painter(Painter const&)            = delete;
    Painter(Painter&&)                 = delete;
    Painter& operator=(Painter const&) = delete;
    Painter& operator=(Painter&&)      = delete;

  private:

    struct ShapeInfo
    {
      virtual ~ShapeInfo() = default;

      std::string name;
      ShapeType   type;
      uint32_t    x = 0;
      uint32_t    y = 0;
      Color       color;
    };

    // canvas can be drawed by multiple shapes
    // you can have multiple canvas
    // every canvas can put on every where on screen
    struct Canvas
    {
      std::vector<std::unique_ptr<ShapeInfo>> shape_infos;

      // put on which window and put position (top left corner)
      class Window const*                     window;
      uint32_t                                x = 0;
      uint32_t                                y = 0;
    };

    struct QuardInfo : public ShapeInfo
    {
      uint32_t width  = 0;
      uint32_t height = 0;
    };

    struct ShapeMatrixInfo
    {
      ShapeType type;
      Color     color;
      glm::mat4 matrix;
    };

  public:
    /** 
     * create a canvas
     * a canvas begin with top left corner which is (0, 0)
     * and x point to right, y point to down
     * will minus space is also valid
     * @param name name of canvas
     * @throw std::runtime_error the name of canvas is existed
     */
    auto create_canvas(std::string_view name) -> Painter&;

    /**
     * select use which one canvas
     * @throw std::runtime_error if not exist canvas be selected
     */
    auto use_canvas(std::string_view name) -> Painter&;

    /**
     * put using canvas on which window and put position
     * xy is left top corner of window
     * @param window put on window
     * @param x
     * @param y
     */
    auto put_on(class Window const& window, uint32_t x, uint32_t y) -> Painter&;

    /**
     * draw a quard
     * xy is left top corner of quard
     * @param name
     * @param x
     * @param y
     * @param width
     * @param height
     * @param color
     */
    auto draw_quard(std::string_view name, uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> Painter&;

    // FIX: should be discard
    auto present(std::string_view canvas_name) -> Painter&;

    // TODO: change to prepare_materials(std::span<Materical>)
    //       struct Materical { shapetype, color };
    //       and use get_materials() return _shape_meshs
    auto get_shape_meshs()               const noexcept { return _shape_meshs; }
    // TODO: can change to get_shapes_draw_result, etc... mix with present function
    auto get_canvas_shape_matrix_infos() const noexcept { return _canvas_shape_matrix_infos; }

  private:
    static auto get_quard_matrix(QuardInfo const& info, class Window const& window, uint32_t x, uint32_t y) -> glm::mat4;

  private:
    std::map<std::string, Canvas> _canvases;
    Canvas*                       _canvas = nullptr;

    std::map<ShapeType, std::map<Color, Mesh>>          _shape_meshs;
    std::map<std::string, std::vector<ShapeMatrixInfo>> _canvas_shape_matrix_infos;
  };

}}
