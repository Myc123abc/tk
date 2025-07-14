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
    tk::init("tk", 200, 200,
    {
      "resources/SourceCodePro-Regular.ttf", // load fonts
    });

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
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - ui::get_start_time()).count();
      log::info("playback button clicked, duration: {} ms", duration);
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }

    ui::polygon({{50, 50}, {100, 75}, {75, 100}}, 0xffffffff);

    //ui::text("H", { 0, 30 }, 26.66, 0xff0000ff);
    //ui::text("Hasdad打ｓas", { 5, 30 }, 120, 0xffffffff, false, false, 0xffff00ff);

    ui::union_begin();
    ui::circle({50, 50}, 25);
    ui::polygon({{0, 0}, {100, 75}, {75, 125}});
    ui::path_begin();
    ui::bezier({0,25}, {75,50}, {0, 75});
    ui::line({0,75},  {0, 25});
    ui::path_end();
    ui::circle({75, 75}, 25);
    ui::union_end(0xffff00ff, 1);

    // playback progress
    auto playback_progree_pos = playback_pos1 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    //auto r1 = ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", { 0, 50 }, 120, 0x000000ff, false, false, 0xff0000ff);
    //auto r2 = ui::text("abcdefghijklmnopqrstuvwxyz", { 0, 50 }, 120, 0xffffffff, true, true);
    //ui::rectangle(r1.first, r1.second, 0xff0000ff, 3);
    //ui::rectangle(r2.first, r2.second, 0x00ff00ff, 3);

    std::string str;
    str.push_back(0x0021);
    ui::text(str, {0,0}, 120, 0xffffffff);

    ui::end();
  }
}