//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"
#include "tk/audio/audio.hpp"

#include <thread>
#include <chrono>

#include <Windows.h>

auto playback_pos0 = glm::vec2(5, 5);
auto playback_pos1 = playback_pos0 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos2 = playback_pos0 + glm::vec2(0, 25);
auto playback_btn  = PlayBackButton("playback_btn", playback_pos0, playback_pos1, playback_pos2, 0xffffffff, 1);
bool click = {};

auto event_process() -> type::WindowState;
void render();

int main(int argc, char** argv)
{
  try
  {
    tk::audio::play("assets\\1.16.遠い心.wav");
    //return 0;

    // init main window and engine
    tk::init("tk", 200, 200);

    tk::load_fonts(
    { 
      "assets/NotoSansJP-Regular.ttf",
      "assets/NotoSansSC-Regular.ttf",
    });

    while (true)
    {
      auto res = tk::event_process();
      if (res == type::WindowState::closed)
        break;
      else if (res == type::WindowState::suspended)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      if (::event_process() == type::WindowState::closed)
        break;
      ::render();

      tk::render();
    }

    tk::destroy();
  }
  catch (std::exception const& e)
  {
    log::error(e.what());
    exit(EXIT_FAILURE);
  }
}

auto event_process() -> type::WindowState
{
  using enum type::WindowState;
  using enum type::Key;
  using enum type::KeyState;

  if (tk::get_key(q) == press)
  {
    return closed;
  }
  if (tk::get_key(space) == press)
  { 
    click = !click;
    playback_btn.click();
  }

  return running;
}

void render()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 1000 / 10;
  static bool paused = false;

  {
    ui::begin("AudioPlayer", 0, 0);

    // background
    ui::rectangle({ 0, 0 }, tk::get_window_size(), 0x282C34FF);

    // playback button
    playback_btn.render();

    // playback progress
    auto playback_progree_pos = playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );
    
    if (playback_btn.button())
    {
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }

    static auto drag = false;
    static auto p = glm::vec2{ 50, 50 };
    static auto p2 = glm::vec2{ 100, 100 };

    static auto prev_mp = ui::get_mouse_position();
    auto mp = ui::get_mouse_position();

    auto mouse_state = ui::get_mouse_state();
    if (mouse_state == type::MouseState::left_down &&
        mp.x >= p.x && mp.x <= p2.x &&
        mp.y >= p.y && mp.y <= p2.y)
    {
      drag = true;
    }

    if (drag)
    {
      auto d = mp - prev_mp;
      p += d;
      p2 += d;
      if (mouse_state == type::MouseState::left_up)
      {
        drag = false;
      }
    }

    prev_mp = mp;
    ui::rectangle(p, p2, 0x00ff00ff);
    ShowCursor(FALSE);
    ui::circle(mp, 4, 0xff00ffff);

    ui::end();
  }
}