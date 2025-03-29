#include "Window.hpp"
#include "GraphicsEngine.hpp"
#include "Log.hpp"

using namespace tk;
using namespace tk::graphics_engine;

int main()
{
  try
  {
    Window win(540, 540, "Breakout");
    GraphicsEngine graphicsEngine(win);
    graphicsEngine.run();
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
  }
}
