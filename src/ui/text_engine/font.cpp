#include "font.hpp"
#include "../config.hpp"
#include "text_engine.hpp"

#include FT_OUTLINE_H
#include <hb-ft.h>
#include <msdfgen.h>

using namespace tk;
using namespace tk::ui;
using namespace tk::renderer;

#define check(x, msg) err_if(static_cast<bool>(x), "[TextEngine] {}", msg)

namespace {

struct FtContext
{
  double           scale{};
  msdfgen::Point2  position{};
  msdfgen::Shape   *shape{};
  msdfgen::Contour *contour{};
};

auto ftPoint2(const FT_Vector &vector, double scale) noexcept
{
  return msdfgen::Point2(scale*vector.x, scale*vector.y);
}

auto ftMoveTo(const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  if (!(context->contour && context->contour->edges.empty()))
    context->contour = &context->shape->addContour();
  context->position = ftPoint2(*to, context->scale);
  return 0;
}

auto ftLineTo(const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position)
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto ftConicTo(const FT_Vector *control, const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position)
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, ftPoint2(*control, context->scale), endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto ftCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user) noexcept
{
  FtContext *context = reinterpret_cast<FtContext *>(user);
  auto endpoint = ftPoint2(*to, context->scale);
  if (endpoint != context->position || crossProduct(ftPoint2(*control1, context->scale)-endpoint, ftPoint2(*control2, context->scale)-endpoint))
  {
    context->contour->addEdge(msdfgen::EdgeHolder(context->position, ftPoint2(*control1, context->scale), ftPoint2(*control2, context->scale), endpoint));
    context->position = endpoint;
  }
  return 0;
}

auto get_shape(FT_GlyphSlot glyph, uint glyph_idx, double scale) noexcept
{
  // read outline
  auto shape = msdfgen::Shape{};
  shape.setYAxisOrientation(msdfgen::Y_DOWNWARD);

  auto ctx = FtContext{};
  ctx.scale = scale;
  ctx.shape = &shape;

  auto outline_funcs = FT_Outline_Funcs{};
  outline_funcs.move_to  = &ftMoveTo;
  outline_funcs.line_to  = &ftLineTo;
  outline_funcs.conic_to = &ftConicTo;
  outline_funcs.cubic_to = &ftCubicTo;
  outline_funcs.shift    = 0;
  outline_funcs.delta    = 0;
  auto err = FT_Outline_Decompose(&glyph->outline, &outline_funcs, &ctx);
  assert(!err);

  if (!shape.contours.empty() && shape.contours.back().edges.empty())
    shape.contours.pop_back();

  // validate and normalize shape
  assert(shape.validate());
  shape.normalize();

  return std::move(shape);
}

auto generate_msdf(msdfgen::Shape& shape) noexcept -> std::pair<msdfgen::Bitmap<float, 3>, float2>
{
  if (shape.contours.empty()) return {};

  static auto const px_range      = msdfgen::Range{ Glyph_MSDF_Pixel_Range };
  static auto const scale         = msdfgen::Vector2{ FT_Pixel_Size };
  static auto const range         = px_range / std::min(scale.x, scale.y);
  static auto const sd_zero_value = range.lower != range.upper ? float(range.lower/(range.lower-range.upper)) : .5f;

  static auto const generator_cfg        = msdfgen::MSDFGeneratorConfig{ true };
  static auto const post_err_correct_cfg = msdfgen::MSDFGeneratorConfig{ true,
    msdfgen::ErrorCorrectionConfig{ msdfgen::ErrorCorrectionConfig::DISABLED, msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE }};

  auto bounds    = shape.getBounds();
  auto min_x     = bounds.l + range.lower;
  auto min_y     = bounds.b + range.lower;
  auto max_x     = bounds.r + range.upper;
  auto max_y     = bounds.t + range.upper;
  auto width     = std::ceil((max_x - min_x) * scale.x);
  auto height    = std::ceil((max_y - min_y) * scale.y);
  auto translate = msdfgen::Vector2{ -min_x, -min_y };
  auto offset    = float2{ static_cast<float>(min_x * scale.x), static_cast<float>(-max_y * scale.y) };

  // generate msdf
  auto msdf           = msdfgen::Bitmap<float, 3>(width, height);
  auto transformation = msdfgen::SDFTransformation{ msdfgen::Projection{ scale, translate }, range};
  msdfgen::edgeColoringSimple(shape, 3, 0);
  msdfgen::generateMSDF(msdf, shape, transformation, generator_cfg);

  msdfgen::distanceSignCorrection(msdf, shape, transformation, sd_zero_value, msdfgen::FILL_NONZERO);
  msdfgen::msdfErrorCorrection(msdf, shape, transformation, post_err_correct_cfg);

  return { std::move(msdf), offset };
}

}

namespace tk::ui {

auto Font::init(std::string_view path) noexcept -> uint8
{
  _name = path;

  auto res = FT_New_Face(g_text_engine._ft, path.data(), 0, &_face);
  if (res) return res;
  res = FT_Set_Pixel_Sizes(_face, 0, FT_Pixel_Size);
  if (res) return res;

  _hb_font = hb_ft_font_create(_face, nullptr);

  _ascender = static_cast<float>(_face->ascender) * FT_Pixel_Size / _face->units_per_EM;
  _height   = static_cast<float>(_face->height)   * FT_Pixel_Size / _face->units_per_EM;
  _family   = _face->family_name;

  using enum FontStyle;
  switch (_face->style_flags & 0b11)
  {
  case FT_STYLE_FLAG_ITALIC:
    _style = italic;
    break;
  case FT_STYLE_FLAG_BOLD:
    _style = bold;
    break;
  case FT_STYLE_FLAG_ITALIC | FT_STYLE_FLAG_BOLD:
    _style = italic_bold;
    break;
  default:
    _style = regular;
    break;
  }

  return res;
}

void Font::destroy() const noexcept
{
  hb_font_destroy(_hb_font);
  check(FT_Done_Face(_face), "failed to destroy font");
}

auto Font::generate_msdf_bitmap(uint glyph_idx, GlyphKey key) const noexcept -> MSDFBitmap
{
  auto lock = std::unique_lock{ _mutex };
  // load glyph
  check(FT_Load_Glyph(_face, glyph_idx, FT_LOAD_NO_SCALE), "failed to load glyph no scale");

  // calc scale
  auto scale = 1.0 / (_face->units_per_EM ? _face->units_per_EM : 1);

  // generate msdf bitmap
  auto shape              = get_shape(_face->glyph, glyph_idx, scale);
  lock.unlock();
  auto [msdf, pos_offset] = generate_msdf(shape);

  auto bitmap = MSDFBitmap{};
  bitmap.extent     = { msdf.width(), msdf.height() };
  bitmap.glyph_key  = key;
  bitmap.pos_offset = pos_offset;
  bitmap.data.resize(bitmap.extent.x * bitmap.extent.y * 4);

  auto cnt  = bitmap.extent.x * bitmap.extent.y;
  auto data = static_cast<float*>(msdf);
  for (auto i = 0; i < cnt; ++i)
  {
    auto to = [](float v) -> uint8 { return std::clamp(v, 0.f, 1.f) * 255.f + .5f; };
    bitmap.data[i * 4 + 0] = to(data[i * 3 + 0]);
    bitmap.data[i * 4 + 1] = to(data[i * 3 + 1]);
    bitmap.data[i * 4 + 2] = to(data[i * 3 + 2]);
    bitmap.data[i * 4 + 3] = 255;
  }

  return bitmap;
}

auto Font::get_glyph_advance(uint glyph_idx) const noexcept -> float2
{
  std::lock_guard lock{ _mutex };
  check(FT_Load_Glyph(_face, glyph_idx, FT_LOAD_DEFAULT), "failed to load glyph for advance");
  auto glyph = _face->glyph;
  return { static_cast<float>(glyph->advance.x) / 64, static_cast<float>(glyph->advance.y) / 64 };
}


}
