//
// Painter
//
// a painter can draw on canvas.
// you should create your canvas, then draw.
//
// TODO:
// 1. every draw need a canvas parameter for draw on multiple canvas
//    or use bind, like painter.use_canvas(canvas0);
// 2. can replace clear value to draw background, just draw a quard fullscreen
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
    };

    struct QuardInfo : public ShapeInfo
    {
      uint32_t width  = 0;
      uint32_t height = 0;
    };

    struct ShapeMatrixInfo
    {
      ShapeType type;
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
     * draw a quard
     * xy is center coordinate of quard
     * @param x
     * @param y
     * @param width
     * @param height
     * @param color
     */
    auto draw_quard(uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> Painter&;

    /**
     * exhibit the canvas on window's some position
     * xy is window's left top corner
     * @param canvas_nanme exhibited canvas
     * @param window window be exhibited
     * @param x position x of window
     * @param y position y of window
     * @throw std::runtime_error if canvas not existed 
     */
    auto present(std::string_view canvas_name, class Window const& window, uint32_t x, uint32_t y) -> Painter&;

    // HACK: I think these are bad way
    //       Painter internel create mesh and draw indexed?
    auto get_shape_meshs()               const noexcept { return _shape_meshs; }
    auto get_canvas_shape_matrix_infos() const noexcept { return _canvas_shape_matrix_infos; }

  private:
    static auto get_quard_matrix(QuardInfo const& info, class Window const& window, uint32_t x, uint32_t y) -> glm::mat4;

  private:
    std::map<std::string, Canvas> _canvases;
    Canvas*                       _canvas = nullptr;

    std::map<ShapeType, Mesh>     _shape_meshs;
    std::map<std::string, std::vector<ShapeMatrixInfo>> _canvas_shape_matrix_infos;
  };

}}
