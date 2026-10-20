#include "line_coverage_atlas.hpp"

#include "../engine/copy_engine.hpp"

#include <algorithm>
#include <cassert>

namespace tk::renderer {

void LineCoverageAtlas::init() noexcept
{
  assert(!_image);
  build_pixels();
  _image = g_img_mgr.create(Width, Height, ImageFormat::r8_unorm, ImageType::srv);
  g_copy_engine.copy(Bitmap{ Width, Height, 1, _pixels.data() }, _image);
}

void LineCoverageAtlas::destroy() noexcept
{
  if (_image)
  {
    g_img_mgr.destroy(_image);
    _image = {};
  }
  _pixels.clear();
}

auto LineCoverageAtlas::select(float thickness, float aa_size) const noexcept -> std::optional<Selection>
{
  if (!_image || aa_size <= 0 || thickness < aa_size)
    return {};

  auto const screen_thickness = thickness / aa_size;
  if (screen_thickness > Max_Width)
    return {};

  auto row = 0;
  if (screen_thickness <= Detailed_Width)
  {
    auto const detail = static_cast<int>((screen_thickness - 1.f) * Sample_Count + .5f);
    row = Max_Width + 1 + std::clamp(detail, 0, Detailed_Row_Count - 1);
  }
  else if (screen_thickness < Max_Width - 1.5f)
    row = static_cast<int>(screen_thickness + .5f);
  else
    row = Max_Width - 1;

  auto const& line = _lines[row];
  return Selection
  {
    .image  = _image,
    .uv0    = line.uv0,
    .uv1    = line.uv1,
    .fringe = aa_size * screen_thickness * line.inverse_width,
  };
}

void LineCoverageAtlas::build_pixels() noexcept
{
  _pixels.assign(Width * Height, 0);

  for (auto line_width = 0; line_width <= Max_Width; ++line_width)
  {
    auto const row = line_width;
    auto const left = (Width - line_width) / 2;
    std::fill_n(_pixels.data() + row * Width + left, line_width, uint8{ 0xff });

    auto const u0 = (static_cast<float>(left) - .5f) / Width;
    auto const u1 = (static_cast<float>(left + line_width) + .5f) / Width;
    auto const v  = (static_cast<float>(row) + .5f) / Height;
    _lines[row] =
    {
      .uv0 = { u0, v },
      .uv1 = { u1, v },
      .inverse_width = line_width > 0 ? 1.f / line_width : 0.f,
    };
  }

  auto ramp = std::array<uint8, Sample_Count>{};
  for (auto index = 0; index < Sample_Count; ++index)
    ramp[index] = static_cast<uint8>(static_cast<float>(index) / Sample_Count * 255.f);

  for (auto detail = 0; detail < Detailed_Row_Count; ++detail)
  {
    auto const row = Max_Width + 1 + detail;
    auto const line_width = 1 + detail;
    auto* output = _pixels.data() + row * Width;
    for (auto alpha : ramp) *output++ = alpha;
    for (auto index = 0; index < line_width; ++index) *output++ = 0xff;
    for (auto alpha : ramp | std::views::reverse) *output++ = alpha;

    auto const u0 = .5f / Width;
    auto const u1 = (-.5f + Sample_Count * 2.f + line_width) / Width;
    auto const v  = (static_cast<float>(row) + .5f) / Height;
    _lines[row] =
    {
      .uv0 = { u0, v },
      .uv1 = { u1, v },
      .inverse_width = 1.f / (1.f + static_cast<float>(detail) / Sample_Count),
    };
  }
}

}
