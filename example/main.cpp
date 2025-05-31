//
// test tk library
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"
#include "tk/util.hpp"

#include <chrono>

using namespace tk;

auto playback_pos1 = glm::vec2(5, 5);
auto playback_pos2 = playback_pos1 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos3 = playback_pos1 + glm::vec2(0, 25);

auto width = playback_pos2.x - playback_pos1.x;
auto part  = width / 3;

auto pause_left1 = playback_pos1;
auto pause_left2 = pause_left1 + glm::vec2(part, 0);
auto pause_left3 = pause_left2 + glm::vec2(0, playback_pos3.y);
auto pause_left4 = glm::vec2(pause_left1.x, pause_left3.y);

auto pause_right2 = glm::vec2(playback_pos2.x, playback_pos1.y);
auto pause_right1 = pause_right2 - glm::vec2(part, 0);
auto pause_right3 = pause_right2 + glm::vec2(0, playback_pos3.y);
auto pause_right4 = glm::vec2(pause_right1.x, pause_right3.y);

auto pause_button() -> bool
{
  ui::rectangle(pause_left1, pause_left3, 0xFFFFFFFF, 1.f);
  ui::rectangle(pause_right1, pause_right3, 0xFFFFFFFF, 1.f);
  return ui::click_area("pause button", pause_left1, pause_right3);
}

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
  static bool paused = false;

  {
    ui::begin("AudioPlayer", 0, 0);

    // background
    ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0x282C34FF);

    // playback button
    if (paused)
      paused = !pause_button();
    else
      paused = ui::button("playback button", type::shape::triangle, { playback_pos1, playback_pos2, playback_pos3 }, 0xFFFFFFFF, 1);
      
    // playback progress
    auto playback_progree_pos = playback_pos2 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }

  {
    ui::begin("Interpolation Animation", 50, 50);

    static const auto pos0  = glm::vec2(15);
    static const auto pos1 = glm::vec2(35, 15);
    static const auto pos2 = glm::vec2(35);
    static const auto pos3  = glm::vec2(15, 35);
    
    static auto pos_change_0 = pos0;
    static auto pos_change_1 = pos1;
    static auto pos_change_2 = pos2;
    static auto pos_change_3 = pos3;

    ui::polygon({pos_change_0, pos_change_1, pos_change_2, pos_change_3}, 0x00ff00ff, 1);

    static const auto pos0e  = glm::vec2(0);
    static const auto pos1e = glm::vec2(50, 0);
    static const auto pos2e = glm::vec2(50);
    static const auto pos3e  = glm::vec2(0, 50);

    auto change_time = 1;
    static float rate = 0.f;

    static bool start_change = false;
    if (!start_change)
      start_change = ui::button("ia", type::shape::rectangle, {{60, 0}, {100, 40}}, 0x0000ffff);
    if (start_change && rate < 1.f)
    {
      static auto start_time = std::chrono::high_resolution_clock::now();
      auto current_time = std::chrono::high_resolution_clock::now();
      auto duration = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count());
      rate = duration / change_time / 1000;

      //pos_change_0 = util::lerp(pos0, pos0e, rate);
      pos_change_1 = util::lerp(pos1, pos1e, rate);
      //pos_change_2 = util::lerp(pos2, pos2e, rate);
      //pos_change_3 = util::lerp(pos3, pos3e, rate);

      //auto v1 = pos11e - pos11;
      //auto e1 = v1 * rate;
      //pos_change_1 = pos11 + e1;
      //auto v2 = pos12e - pos12;
      //auto e2 = v2 * rate;
      //pos_change_2 = pos12 + e2;
    }

    //ui::triangle({ 0, 0 }, { 50, 25 }, { 0, 50 }, 0x00ff00ff);

    //ui::rectangle({ 0, 0 }, { 50, 50 }, 0x00ff00ff);

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
