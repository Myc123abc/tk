//
// ui internal lib
//

#pragma once

#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ui/UIWidget.hpp"

#include <SDL3/SDL_events.h>

namespace tk { namespace ui {

  struct ui_context
  {
    // TODO: why static delete is failed
    inline static graphics_engine::GraphicsEngine*       engine;
    inline static std::vector<std::unique_ptr<Layout>>   layouts;
    inline static std::vector<std::unique_ptr<UIWidget>> widgets;

    // background in layouts[0]
    // default use onedark background
    inline static UIWidget* background = nullptr;

    inline static ClickableWidget* mouse_down_widget = nullptr;
  };

  inline auto& get_ctx()
  {
    static ui_context ctx;
    return ctx;
  }

  void event_process(SDL_Event* event);

}}
