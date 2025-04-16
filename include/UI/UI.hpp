//
// UI library
//
// implement by graphics engine
//

#pragma once

namespace tk { namespace ui {

  class UI
  {
  public:
    static void init(class GraphicsEngine& engine);
    static void destroy();

  private:
    UI()  = delete;
    ~UI() = delete;

    UI(UI const&)            = delete;
    UI(UI&&)                 = delete;
    UI& operator=(UI const&) = delete;
    UI& operator=(UI&&)      = delete;

  private:
    class GraphicsEngine* _engine = nullptr;
  };

}}
