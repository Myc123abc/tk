#include "tk/ui/Font.hpp"
#include "tk/ErrorHandling.hpp"

#include <msdfgen.h>
#include <msdfgen-ext.h>

namespace tk { namespace ui {

void Font::init(std::filesystem::path const& path)
{
  throw_if(!std::filesystem::exists(path), "{} not exist");

  auto ft = msdfgen::initializeFreetype();
  throw_if(!ft, "failed to init freetype");
  auto font = loadFont(ft, path.string().c_str());
  throw_if(!font, "failed to load font {}", path.string());
          //Shape shape;
          //if (loadGlyph(shape, font, 'A', FONT_SCALING_EM_NORMALIZED)) {
          //    shape.normalize();
          //    //                      max. angle
          //    edgeColoringSimple(shape, 3.0);
          //    //          output width, height
          //    Bitmap<float, 3> msdf(32, 32);
          //    //                            scale, translation (in em's)
          //    SDFTransformation t(Projection(32.0, Vector2(0.125, 0.125)), Range(0.125));
          //    generateMSDF(msdf, shape, t);
          //    //savePng(msdf, "output.png");
  destroyFont(font);
  deinitializeFreetype(ft);
}

}}