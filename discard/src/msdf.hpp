#pragma once

#include "util/error_handling.hpp"
#include "util/base.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <msdfgen.h>

#include <cassert>

using namespace tk;

namespace {

struct FtContext {
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

}

inline static constexpr auto PxSize  = 32;
inline static const     auto pxRange = msdfgen::Range{ 2 };

inline auto get_shape(FT_GlyphSlot glyph, uint glyph_idx, double scale) noexcept
{
  // read outline
  auto shape = msdfgen::Shape{};
  // shape.setYAxisOrientation(msdfgen::Y_UPWARD);
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
  auto y_flip = false;
  if (y_flip)
    shape.inverseYAxis = !shape.inverseYAxis;

  return std::move(shape);
}

inline auto load_glyph(FT_Face face, uint unicode) noexcept -> std::pair<double, msdfgen::Shape>
{
  auto glyph_idx = FT_Get_Char_Index(face, unicode);
  assert(glyph_idx);

  // load glyph
  auto err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_NO_SCALE);
  assert(!err);

  // calc advance
  auto scale   = 1.0 / 64;
  return { scale * face->glyph->advance.x, get_shape(face->glyph, glyph_idx, scale) };
}

inline auto generate_msdf(msdfgen::Shape& shape) noexcept -> msdfgen::Bitmap<float, 3>
{
  auto range     = msdfgen::Range{ 1 };
  auto translate = msdfgen::Vector2{};
  auto scale     = msdfgen::Vector2{ 1 };

  auto px_translate = msdfgen::Vector2{};

  translate += px_translate / scale;

  auto avgScale = .5 * (scale.x + scale.y);

  auto bounds = shape.getBounds();

  // auto frame
  double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
  auto frame = msdfgen::Vector2(PxSize, PxSize);
  frame += 2 * pxRange.lower;
  if (l >= r || b >= t)
    l = 0, b = 0, r = 1, t = 1;
  err_if(frame.x <= 0 || frame.y <= 0, "Cannot fit the specified pixel range.");

  auto dims = msdfgen::Vector2(r-l, t-b);
  if (dims.x*frame.y < dims.y*frame.x)
  {
    translate.set(.5*(frame.x/frame.y*dims.y-dims.x)-l, -b);
    scale = avgScale = frame.y/dims.y;
  } else
  {
    translate.set(-l, .5*(frame.y/frame.x*dims.x-dims.y)-b);
    scale = avgScale = frame.x/dims.x;
  }
  translate -= pxRange.lower/scale;
  range = pxRange/std::min(scale.x, scale.y);

  // generate msdf
  auto transformation = msdfgen::SDFTransformation{ msdfgen::Projection{ scale, translate }, range};
  auto generator_cfg = msdfgen::MSDFGeneratorConfig{};
  generator_cfg.overlapSupport = true;
  auto post_err_correct_cfg = generator_cfg;
  generator_cfg.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::DISABLED;
  post_err_correct_cfg.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;

  msdfgen::edgeColoringSimple(shape, 3, 0);
  auto msdf = msdfgen::Bitmap<float, 3>(PxSize, PxSize);
  msdfgen::generateMSDF(msdf, shape, transformation, generator_cfg);

  float sdfZeroValue = range.lower != range.upper ? float(range.lower/(range.lower-range.upper)) : .5f;
  msdfgen::distanceSignCorrection(msdf, shape, transformation, sdfZeroValue, msdfgen::FILL_NONZERO);
  msdfgen::msdfErrorCorrection(msdf, shape, transformation, post_err_correct_cfg);

  return std::move(msdf);
}

inline void test_msdfgen() noexcept
{
  // init freetype
  auto ft_lib = FT_Library{};
  auto err = FT_Init_FreeType(&ft_lib);
  assert(!err);

  // load font
  auto ft_face = FT_Face{};
  err = FT_New_Face(ft_lib, "assets/font/NotoSansJP-Regular.ttf", 0, &ft_face);
  assert(!err);

  // load glyph
  auto [advance, shape] = load_glyph(ft_face, 0x89a7);

  auto msdf = generate_msdf(shape);

  // output png
  std::vector<uint8_t> png(PxSize * PxSize * 3);
  float *src = static_cast<float *>(msdf);
  for (int i = 0; i < PxSize * PxSize * 3; ++i)
  {
    png[i] = static_cast<uint8_t>(
      std::clamp(src[i], 0.0f, 1.0f) * 255.0f + 0.5f);
  }

  stbi_write_png("test.png", PxSize, PxSize, 3, png.data(), PxSize * 3);

  FT_Done_Face(ft_face);
  FT_Done_FreeType(ft_lib);
}
