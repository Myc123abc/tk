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
  TK_API void init(std::string_view title, uint32_t width, uint32_t height);

  TK_API auto get_window_size() -> glm::vec2;

  TK_API auto event_process() -> type::window;

  TK_API void render();
  
  TK_API void destroy();

  TK_API auto get_key(type::key k) -> type::key_state;
}
