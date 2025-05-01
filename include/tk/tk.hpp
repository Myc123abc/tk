//
// tk
//

#pragma once

#include <string_view>

#include <SDL3/SDL_init.h>

namespace tk
{
  
  /**
   * initialize tk context
   * @param title title of main window
   * @param width width of main window
   * @param height height of main window
   * @param user_data user_data pointer
   */
  void init_tk_context(std::string_view title, uint32_t width, uint32_t height, void* user_data);

  auto get_user_data() -> void*;

}
