#include "frame_data.hpp"

namespace tk::ui {

void FrameData::add_rect(glm::vec2 left_top, glm::vec2 right_bottom, Color color, float thickness) noexcept
{
  auto [vertices, indices] = expand_beg(4, 6);

  vertices[0] = { left_top, color };
  vertices[1] = { { right_bottom.x, left_top.y }, color };
  vertices[2] = { right_bottom, color };
  vertices[3] = { { left_top.x, right_bottom.y }, color };
  indices[0]  = _index;
  indices[1]  = _index + 1;
  indices[2]  = _index + 2;
  indices[3]  = _index;
  indices[4]  = _index + 2;
  indices[5]  = _index + 3;

  expand_end();
}

void FrameData::add_scissor_rect(RECT rect) noexcept
{
  _draw_datas.emplace_back(rect, _draw_index_beg, _indices.size() - _draw_index_beg);
  _draw_index_beg = _indices.size();
}

}
