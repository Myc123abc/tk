//
// tk
//

#pragma once

#include "config.hpp"
#include "type.hpp"

#include <vector>
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

  /**
   * must call tk::render after calling this function,
   * because tk::render render first frame then set window visible.
   * if not call tk::render, the window will not display and event_process will alway return type::window::running
   * I do this just for display window the first frame content to avoid the possible temporary blank 
   */
  TK_API auto event_process() -> type::WindowState;

  TK_API void render();
  
  TK_API void destroy();

  TK_API auto get_key(type::Key k) -> type::KeyState;

  /**
   * load fonts, can dynamic load
   * @param fonts
   */
  TK_API void load_fonts(std::vector<std::string_view> fonts);
}
