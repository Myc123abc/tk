//
// test tk library
//

#include "PlayBackButton.hpp"
#include "tk/tk.hpp"
#include "tk/log.hpp"

#include <thread>
#include <chrono>

auto playback_pos0 = glm::vec2(5, 5);
auto playback_pos1 = playback_pos0 + glm::vec2(12.5 * 1.414, 12.5);
auto playback_pos2 = playback_pos0 + glm::vec2(0, 25);
auto playback_btn  = PlayBackButton("playback_btn", playback_pos0, playback_pos1, playback_pos2, 0xffffffff, 1);
bool click = {};

auto event_process() -> type::window;
void render();

int main()
{
  try
  {
    // init main window and engine
    tk::init("tk", 200, 200);

    while (true)
    {
      auto res = tk::event_process();
      if (res == type::window::closed)
        break;
      else if (res == type::window::suspended)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      if (::event_process() == type::window::closed)
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

auto event_process() -> type::window
{
  using enum type::window;
  using enum type::key;
  using enum type::key_state;

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
    if (playback_btn.button())
    {
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }

    // playback progress
    auto playback_progree_pos = playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );
    /*
    TODO:
      1. change text_mask_image from R32 to R32G32
        R32 -> signed distance
        G32 -> RGBA
        enable alpha mix
        sdf render text use color from text_mask_image's G32
      
      2. text outline
    */
    //ui::text("H", { 0, 30 }, 26.66, 0xff0000ff);
    //ui::text("H", { 5, 30 }, 26.66, 0x00ff004f);
    
    ui::end();
  }
}