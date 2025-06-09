//
// tk
//

#pragma once

#include "config.hpp"
#include "tk/type.hpp"

#include <string_view>

#include <glm/glm.hpp>

namespace tk
{
  /**
   * initialize tk context
   * @param title title of main window
   * @param width width of main window
   * @param height height of main window
   */
  TK_API void tk_init(std::string_view title, uint32_t width, uint32_t height);

  TK_API auto get_main_window_extent() -> glm::vec2;

  TK_API void tk_poll_events();

  TK_API auto tk_event_process() -> type::window;

  TK_API void tk_render();
  
  TK_API void tk_destroy();

  TK_API auto tk_get_key(type::key k) -> type::key_state;
}
