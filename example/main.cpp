//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"

#include <chrono>

using namespace tk;

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 200, nullptr);
}

void tk_iterate()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 10000 / 100;

  //log::info("progress {}%", progress);

  ui::begin("background", 0, 0);
  ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0x282C34FF);
  ui::end();

  {
    ui::begin("l1", 5, 5);

    // playback button
    auto playback_right_point = glm::vec2{ 12.5 * 1.414, 12.5 };
    auto color1 = 0xFFFFFFFF;
    auto color2 = 0xFFFF00FF;
    static auto color = color1;
    static auto present = true;
    if (ui::button("btn1", type::shape::triangle, { {}, playback_right_point, { 0, 25 } }, color))
    {
      if (color == color1) color = color2;
      else                 color = color1;
      present = !present;
    }

    if (present)
    {
      static auto color_a = color1;
      playback_right_point += 50;
      if (ui::button("btn2", type::shape::triangle, { { 50, 50 }, playback_right_point, { 55, 80 } }, color_a))
      {
        if (color_a == color1) color_a = color2;
        else                   color_a = color1;      
      }
    }

    ui::triangle({5,5}, {195,5}, {5, 195}, 0x00ff00ff, 1.f);

    // playback progress
    ui::rectangle({ playback_right_point.x + 5, playback_right_point.y }, { 100, 3 }, 0x808080FF );
    ui::rectangle({ playback_right_point.x + 5, playback_right_point.y }, { progress, 3 }, 0x0000FFFF );

    ui::path_line_to({ 20, 20 });
    ui::path_line_to({ 70, 20 });
    ui::path_line_to({ 150, 70 });
    ui::path_line_to({ 150, 150 });
    ui::path_line_to({ 80, 180 });
    ui::path_line_to({ 20, 50 });
    ui::path_stroke(0xFFFFFFFF, 20.f, true);

    ui::end();
  }

  {
    ui::begin("l2", 50, 0);
    auto playback_right_point = glm::vec2{ 5 + 12.5 * 1.414, 17.5 };
    auto color1 = 0xFFFFFFFF;
    auto color2 = 0xFFFF00FF;
    static auto color = color1;
    if (ui::button("btn1", type::shape::triangle, { { 5, 5 }, playback_right_point, { 5, 30 } }, color, 1.f))
    {
      if (color == color1) color = color2;
      else                 color = color1;      
    }
    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
