//
// test tk library
//

#include "tk/tk.hpp"
#include "tk/ui/ui.hpp"
#include "tk/log.hpp"
#include "tk/util.hpp"
#include "tk/ui/LerpPoint.hpp"

#include <chrono>

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

class PlaybackButton final
{
public:
  PlaybackButton(std::string const& name, glm::vec2 const& p0, glm::vec2 const& p1, glm::vec2 const& p2)
  {
    _playback_btn_name = name + "play";
    _pause_btn_name    = name + "pause";

    _positions[0] = p0;
    _positions[3] = p2;
    _positions[5] = p1;
    _positions[6] = _positions[5];
    _part = (_positions[5] - _positions[0]) / 2.f;
    _positions[1] = _positions[0] + _part;
    _positions[4] = _positions[1];
    _positions[2] = glm::vec2(_positions[3].x + _part.x, _positions[3].y - _part.y);
    _positions[7] = _positions[2];

    _playback_pos[0] = _positions[0];
    _playback_pos[1] = _positions[5];
    _playback_pos[2] = _positions[3];

    _pause_pos[0] = _positions[0];
    auto part = (_positions[5].x - _positions[0].x) / 3.f;
    _pause_pos[1] = _pause_pos[0] + glm::vec2(part, 0);
    _pause_pos[2] = _pause_pos[1] + glm::vec2(0, _part.y * 4);
    _pause_pos[3] = _pause_pos[0] + glm::vec2(0, _part.y * 4);
    _pause_pos[5] = _positions[0] + glm::vec2(_part.x * 2, 0);
    _pause_pos[4] = _pause_pos[5] - glm::vec2(part, 0);
    _pause_pos[6] = _pause_pos[5] + glm::vec2(0, _part.y * 4);
    _pause_pos[7] = _pause_pos[4] + glm::vec2(0, _part.y * 4);

    for (auto i = 0; i < 8; ++i)
      _positions2[i] = _pause_pos[i];
  }

  enum
  {
    playing,
    paused,
    animation,
  };

  auto pause_button()
  {
    ui::rectangle(_pause_pos[0], _pause_pos[2], 0xFFFFFFFF, 1.f);
    ui::rectangle(_pause_pos[4], _pause_pos[6], 0xFFFFFFFF, 1.f);
    return ui::click_area(_pause_btn_name, _pause_pos[0], _pause_pos[6]);
  }

  void render()
  {
    if (_status == playing)
    {
      if (pause_button())
      {
        _status = paused;
      }
    }
    else if (_status == paused)
    {
      if (ui::button(_playback_btn_name, type::shape::triangle, { _playback_pos[0], _playback_pos[1], _playback_pos[2] }, 0xFFFFFFFF, 1))
      {
        _status = playing;
      }
    }
    else if (_status == animation)
    {

    }
  }

  auto status() const noexcept { return _status; }

private:
  std::string _playback_btn_name;
  std::string _pause_btn_name;
  glm::vec2   _positions[8];
  glm::vec2   _positions2[8];
  glm::vec2   _playback_pos[3];
  glm::vec2   _pause_pos[8];
  glm::vec2   _part;
  uint32_t    _status;
};

////////////////////////////////////////////////////////////////////////////////
//                               tk callback
////////////////////////////////////////////////////////////////////////////////

PlaybackButton playback_btn("playback_btn", { 5, 5 }, { 5 + 12.5 * 1.414, 5 + 12.5 }, { 5, 5 + 25 });
LerpPoint lp({0, 100}, {100, 100}, 1000);

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
    playback_btn.render();
    
    if (ui::button("lp btn", type::shape::circle, { glm::vec2(75), glm::vec2(25) }, 0xffffffff))
    {
      if (lp.now() == lp.end())
        lp.reverse();
      lp.run();
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
    ui::line(lp.start(), lp.end(), 0xffffffff);
    ui::circle(lp.now(), 2, 0xff000000);
    //if (lp.single_finished())
    //{
    //  log::info("finished");
    //}

    ui::end();
  }
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
}
