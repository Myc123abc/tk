#pragma once

#include "../shader/sdf/type.hpp"
#include "../../ui/command_list.hpp"

#include <d3d12.h>
#include <windows.h>

#include <vector>

namespace tk { namespace renderer {

struct DrawData
{
  uint32_t                    indices_size{};
  uint32_t                    indices_start{};
  float                       image_alpha{};
  D3D12_GPU_DESCRIPTOR_HANDLE image_descriptor_gpu_handle{};
  bool                        is_image{};
};

struct RenderData
{
  std::vector<Vertex>        vertices;
  std::vector<uint16_t>      indices;
  std::vector<ShapeProperty> shape_properties;
  RECT                       scissor_rect{};
  glm::vec2                  resizing_window_pos;
  std::vector<DrawData>      draw_datas;

  void clear() noexcept
  {
    vertices.clear();
    indices.clear();
    shape_properties.clear();
    scissor_rect        = {};
    resizing_window_pos = {};
    draw_datas.clear();
    _prev_is_image_type = {};
    _last_indices_size  = {};
  }

  void try_push_draw_data(ui::CommandType type) noexcept
  {
    auto current_is_image_type = type == ui::CommandType::image;
    if (draw_datas.empty()) draw_datas.emplace_back(DrawData{});
    else if (_prev_is_image_type || current_is_image_type)
    {
      draw_datas.back().indices_size = indices.size() - _last_indices_size;
      _last_indices_size = indices.size();
      draw_datas.emplace_back(DrawData{});
      draw_datas.back().indices_start = indices.size();
    }
    _prev_is_image_type = current_is_image_type;
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

  void set_image_info(D3D12_GPU_DESCRIPTOR_HANDLE handle, float alpha) noexcept
  {
    auto& data = draw_datas.back();
    data.is_image                    = true;
    data.image_descriptor_gpu_handle = handle;
    data.image_alpha                 = alpha;
  }

private:
  bool     _prev_is_image_type{};
  uint32_t _last_indices_size{};
};

}}
