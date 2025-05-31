//
// test tk library
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"
#include "tk/util.hpp"
#include "tk/ui/LerpPoint.hpp"

#include <chrono>
#include <span>

using namespace tk;
using namespace tk::ui;

auto playback_pos1 = glm::vec2(5, 5);
auto playback_pos4 = playback_pos1 + glm::vec2(0, 25);
auto playback_pos6 = playback_pos1 + glm::vec2(12.5 * 1.414, 12.5);

auto width = playback_pos6.x - playback_pos1.x;
auto part  = width / 3;

auto pause_left1 = playback_pos1;
auto pause_left2 = pause_left1 + glm::vec2(part, 0);
auto pause_left3 = pause_left2 + glm::vec2(0, playback_pos4.y);
auto pause_left4 = glm::vec2(pause_left1.x, pause_left3.y);

auto pause_right2 = glm::vec2(playback_pos6.x, playback_pos1.y);
auto pause_right1 = pause_right2 - glm::vec2(part, 0);
auto pause_right3 = pause_right2 + glm::vec2(0, playback_pos4.y);
auto pause_right4 = glm::vec2(pause_right1.x, pause_right3.y);

//ui::polygon({ playback_pos1, playback_pos2, playback_pos3, playback_pos4 }, 0xffffffff, 1);
//ui::polygon({ playback_pos5, playback_pos6, playback_pos7, playback_pos8 }, 0xffffffff, 1);

////////////////////////////////////////////////////////////////////////////////
//                             lerp animation
////////////////////////////////////////////////////////////////////////////////

class PlayBackButton
{
public:
  PlayBackButton(std::string const& name, glm::vec2 p0, glm::vec2 p1, glm::vec2 p2)
  {
    _playback_btn_name = name + "playback";
    _pasue_btn_name    = name + "pause";

    auto rect = glm::vec2(p1.x - p0.x, (p1.y - p0.y) * 2);

    _lps.reserve(8);
    _lps.append_range(std::vector<LerpPoint>
    {
      // pause button                       // playback buttin
      { p0,                                 p0,                                      100 },
      { p0 + glm::vec2( rect.x / 3, 0),     p0 + glm::vec2(rect.x / 2,  rect.y / 4), 100 },
      { p2 + glm::vec2( rect.x / 3, 0),     p2 + glm::vec2(rect.x / 2, -rect.y / 4), 100 },
      { p2,                                 p2,                                      100 },
      { glm::vec2(p1.x - rect.x / 3, p0.y), p0 + glm::vec2(rect.x / 2,  rect.y / 4), 100 },
      { glm::vec2(p1.x, p0.y),              p1,                                      100 },
      { glm::vec2(p1.x, p2.y),              p1,                                      100 },
      { glm::vec2(p1.x - rect.x / 3, p2.y), p2 + glm::vec2(rect.x / 2, -rect.y / 4), 100 },
    });

    _left_upper  = p0;
    _right_lower = _lps[6].start();
  }

  auto button() -> bool
  {
    if (ui::click_area("test", _left_upper, _right_lower))
    {
      if (_lps[1].end() == _lps[1].now())
      {
        for (auto& p : _lps)
          p.reverse();
      }

      for (auto& p : _lps)  
        p.run();

      return true;
    } 
    return false;
  }

  void render()
  {
    for (auto& p : _lps)
      p.render();

    auto rect0 = std::span(_lps.begin(), _lps.begin() + 4);
    auto rect1 = std::span(_lps.begin() + 4, _lps.end());
    ui::polygon({ rect0.begin(), rect0.end() }, 0xffffffff, 1);
    ui::polygon({ rect1.begin(), rect1.end() }, 0xffffffff, 1);
  }

private:
  std::vector<LerpPoint> _lps;
  std::string _playback_btn_name, _pasue_btn_name;
  glm::vec2 _left_upper, _right_lower;
};

////////////////////////////////////////////////////////////////////////////////
//                               tk callback
////////////////////////////////////////////////////////////////////////////////

PlayBackButton playback_btn("playback_btn", { 5, 5 }, { 5 + 12.5 * 1.414, 5 + 12.5 }, { 5, 5 + 25 });
LerpPoint lp({0, 100}, {100, 100}, 1000);
LerpPoints lps(std::vector<LerpInfo>
{
  { { 0, 150 }, { 100, 150 }, 1000 },
  { { 100, 150 }, { 100, 250 }, 1000 },
});

void tk_init(int argc, char** argv)
{
  init_tk_context("tk", 200, 350, nullptr);
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
    playback_btn.render();
    if (playback_btn.button())
    {
      log::info("click");
    }
    
    if (ui::button("lp btn", type::shape::circle, { glm::vec2(75), glm::vec2(25) }, 0xffffffff))
    {
      if (lp.now() == lp.end())
        lp.reverse();
      lp.run();

      lps.run();
    }

    //if (paused)
    //  paused = !pause_button();
    //else
    //  paused = ui::button("playback button", type::shape::polygon, { playback_pos1, playback_pos4, playback_pos6 }, 0xFFFFFFFF, 1);
      
    // playback progress
    auto playback_progree_pos = playback_pos6 + glm::vec2(5, 0);
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ 100, 3 }, 0x808080FF );
    ui::rectangle(playback_progree_pos, playback_progree_pos + glm::vec2{ progress, 3 }, 0x0000FFFF );

    ui::end();
  }

  {
    ui::begin("Interpolation Animation", 50, 50);
 
    lp.render();
    ui::line({0, 100}, {100, 100}, 0xffffffff);
    ui::circle(lp.now(), 2, 0xff000000);
    //if (lp.single_finished())
    //{
    //  log::info("finished");
    //}

    lps.render();
    ui::line({ 0, 150 }, { 100, 150 }, 0xffffffff);
    ui::line({ 100, 150 }, { 100, 250 }, 0xffffffff);
    ui::circle(lps.now(), 2, 0xff000000);

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
