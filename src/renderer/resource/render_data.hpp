#pragma once

#include "shader_type.hpp"
#include "../../ui/command_list.hpp"
#include "../renderer/pipeline/pipeline_system.hpp"

#include <d3d12.h>
#include <windows.h>

#include <vector>
#include <queue>

namespace tk::renderer {

struct DrawData
{
  uint32_t     indices_size{};
  uint32_t     indices_start{};
  float        image_alpha{};
  Image*       image{};
  PipelineType pipe_type{};
};

struct RenderData
{
  std::vector<Vertex>                 vertices;
  std::vector<uint16_t>               indices;
  std::vector<ShapeProperty>          shape_properties;
  glm::vec2                           resizing_window_pos;
  std::vector<DrawData>               draw_datas;
  std::optional<ui::WindowShadowInfo> window_shadow_info;

  void clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
    resizing_window_pos = {};
    draw_datas.clear();
    _prev_is_image_type = {};
    _last_indices_size  = {};
    assert(_scissor_infos.empty());
    window_shadow_info = {};
  }

  void try_push_draw_data(ui::CommandType type) noexcept
  {
    auto current_is_image_type = type == ui::CommandType::image;
    if (draw_datas.empty()) draw_datas.emplace_back(DrawData{});
    else if (_prev_is_image_type || current_is_image_type)
      push_draw_data();
    _prev_is_image_type = current_is_image_type;
  }

  void push_draw_data() noexcept
  {
    if (draw_datas.empty()) draw_datas.emplace_back(DrawData{});
    draw_datas.back().indices_size = indices.size() - _last_indices_size;
    _last_indices_size = indices.size();
    draw_datas.emplace_back(DrawData{});
    draw_datas.back().indices_start = indices.size();
  }

  void generate_finish() noexcept
  {
    if (!draw_datas.empty())
    {
      draw_datas.back().indices_size = indices.size() - _last_indices_size;
      if (indices.size() == _last_indices_size)
        draw_datas.pop_back();
    }
  }

  void set_image_info(Image* image, float alpha) noexcept
  {
    auto& data = draw_datas.back();
    data.pipe_type   = PipelineType::image;
    data.image       = image;
    data.image_alpha = alpha;
  }

  void push_scissor_rect(RECT rect) noexcept
  {
    _scissor_infos.emplace(rect, indices.size());
  }

  auto pop_scissor_rect() noexcept
  {
    auto info = _scissor_infos.front();
    _scissor_infos.pop();
    return info;
  }

  auto is_scissor_rect_empty() const noexcept { return _scissor_infos.empty(); }

private:
  bool     _prev_is_image_type{};
  uint32_t _last_indices_size{};

  struct ScissorInfo
  {
    RECT     rect{};
    uint32_t next_indices_idx;
  };
  std::queue<ScissorInfo> _scissor_infos;
};

}
