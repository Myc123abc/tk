#pragma once

#include "image_manager.hpp"
#include "../../util/singleton.hpp"

#include <array>
#include <optional>
#include <vector>

namespace tk::renderer {

Singleton(LineCoverageAtlas, g_line_coverage_atlas,
public:
  struct Selection
  {
    ImageHandle image;
    float2      uv0;
    float2      uv1;
    float       fringe;
  };

  void init() noexcept;
  void destroy() noexcept;

  auto select(float thickness, float aa_size) const noexcept -> std::optional<Selection>;
  auto image() const noexcept { return _image; }

  inline static constexpr auto Max_Width          = 32;
  inline static constexpr auto Detailed_Width     = 4;
  inline static constexpr auto Sample_Count       = 4;
  inline static constexpr auto Detailed_Row_Count = (Detailed_Width - 1) * Sample_Count + 1;
  inline static constexpr auto Width              = Max_Width + 2;
  inline static constexpr auto Height             = Max_Width + 1 + Detailed_Row_Count;

private:
  struct LineInfo
  {
    float2 uv0;
    float2 uv1;
    float  inverse_width{};
  };

  void build_pixels() noexcept;

  ImageHandle                    _image;
  std::vector<uint8>             _pixels;
  std::array<LineInfo, Height>   _lines;
)

}
