//
// tk
//

#pragma once

#include "config.hpp"

#include <string_view>

#include <glm/glm.hpp>

#include <SDL3/SDL_events.h>

namespace tk
{
  /**
   * initialize tk context
   * @param title title of main window
   * @param width width of main window
   * @param height height of main window
   */
  TK_API void init_tk_context(std::string_view title, uint32_t width, uint32_t height);

  TK_API auto get_main_window_extent() -> glm::vec2;

  // TODO: tmp
  TK_API bool tk_resize();
  TK_API void tk_render();
  TK_API void tk_set_resize();
  TK_API void tk_set_event(SDL_Event *event);
  TK_API void tk_quit();
}
