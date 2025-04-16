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

#include "MaterialLibrary.hpp"

#include <vector>
#include <string_view>
#include <map>
#include <memory>

namespace tk { namespace graphics_engine {


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

      uint32_t    id;
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
     * put canvas on specific position of window
     * xy is left top corner of window
     * @param canvas use which canvas
     * @param window put on window
     * @param x
     * @param y
     */
    auto put(std::string_view canvas, class Window const& window, uint32_t x, uint32_t y) -> Painter&;

    /**
     * draw a quard
     * xy is left top corner of quard
     * @param x
     * @param y
     * @param width
     * @param height
     * @param color
     * @return shape id
     */
    auto draw_quard(uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> uint32_t;

    /**
     * redraw a quard
     * xy is left top corner of quard
     * @param id
     * @param x
     * @param y
     * @param width
     * @param height
     * @param color
     * @throw if id is invalid
     */
    auto redraw_quard(uint32_t id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, Color color) -> Painter&;

    // TODO: expand to can specific name of shape to generate its matrix info
    //       and specific canvases to generate
    /**
     * generate shape matrix infos of all canvases
     */
    auto generate_shape_matrix_info_of_all_canvases() -> Painter&;

    /**
     * get shape matrix infos of all canvases
     */
    auto get_shape_matrix_info_of_all_canvases() const noexcept { return _canvas_shape_matrix_infos; }

  private:
    static auto generate_id() noexcept { return ++_id; }

    static auto get_quard_matrix(QuardInfo const& info, class Window const& window, uint32_t x, uint32_t y) -> glm::mat4;

  private:
    std::map<std::string, Canvas> _canvases;
    Canvas*                       _canvas = nullptr;

    std::map<std::string, std::vector<ShapeMatrixInfo>> _canvas_shape_matrix_infos;

    // global id
    inline static uint32_t _id = -1;
  };

}}
