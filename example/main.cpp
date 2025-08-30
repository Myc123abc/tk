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

auto event_process() -> type::WindowState;
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

    // TODO: space, invalid character, cache, mulitple atlases, multiple fonts
    auto baseline = glm::vec2{ 0, 50 };
    
    //ui::text("ABCDEFｇがIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 30 }, 32, 0xffffffff, type::FontStyle::regular);
    //ui::line(baseline + glm::vec2 {0, 30}, baseline + glm::vec2 {1000, 30}, 0x00ff00ff);

    ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline, 32, 0xffffffff);
    ui::text("ABCDEFｇがIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 30 }, 32, 0xffffffff, type::FontStyle::regular);
    ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 60 }, 32, 0xffffffff, type::FontStyle::bold);
    ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 90 }, 32, 0xffffffff, type::FontStyle::italic_bold);    

    ui::text("老妈马骢", baseline + glm::vec2{ 0, 150 }, 128, 0xffffffff, 0xff0000ff);
    ui::text("老妈马骢", baseline + glm::vec2{ 0, 300 }, 128, 0xffffffff, 0xff0000ff);
    //ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 150 }, 128, 0xffffffff, 0xff000000);
    //ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 300 }, 128, 0xffffffff, 0xff000000);
    //ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 175 }, 128, 0xffffffff, true, false);
    //ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 375 }, 256, 0xffffffff, false, true, 0xff0000ff);
    //auto r1 = ui::text("abcdefghijklmnopqrstuvwxyz", baseline, 32, 0x00ff00ff, false, false);
    //auto r2 = ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", baseline + glm::vec2{ 0, 50 }, 32, 0xffffffff, false, true);
    //auto r3 = ui::text("あいうえおかきくけこたちつてとなにぬねのまみむめもはひふへほかちくけこらりるれろやみむめもん", baseline + glm::vec2{ 0, 100 }, 32, 0xffffffff, false, false);
    //auto r4 = ui::text("アイウエオカキクケコタチツテトナニヌネノマミムメモハヒフヘホカチケケコラリルレロヤミムマモン", baseline + glm::vec2{ 0, 150 }, 32, 0xffffffff, false, false);
    //auto r5 = ui::text("我草，我真是服了啊，明天又要上班。害", baseline + glm::vec2{ 0, 200 }, 32, 0xffffffff, false, false);
    //ui::line(baseline, { 1000, baseline.y }, 0x00ff00ff);
    // auto r1 = ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", { 0, 0 }, 120, 0xffffffff, false, false);
    // ui::text("abcdefghijklmnopqrstuvwxyz", { 0, r1.second.y }, 120, 0xffffffff, false, false);
    //auto r1 = ui::text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", { 0, 50 }, 120, 0x000000ff, false, false, 0xff0000ff);
    //auto r1 = ui::text("aaa きゃあ我是谁？XX骢 cas", { 0, 150 }, 120, 0xffffffff, false, false);
    //auto r2 = ui::text("aあcdefghijklmnopqrstuvwxyz", { 0, r1.second.y }, 120, 0xffffffff, true, false);
    //auto r2 = ui::text("aaa きゃあ我是谁？XX骢 cas", { 0, r1.second.y }, 120, 0xffffffff, false, true);
    //ui::rectangle(r1.first, r1.second, 0xff0000ff, 3);
    //ui::rectangle(r2.first, r2.second, 0x00ff00ff, 3);

    //std::string str;
    //str.push_back(0x0021);
    //ui::text(str, {0,0}, 120, 0xffffffff);

    if (playback_btn.button())
    {
      click = !click;
    }
    if (click)
    {
      ui::circle({50, 50}, 10, 0x00ff00ff);
    }

    ui::end();
  }
}