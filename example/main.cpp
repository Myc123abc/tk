//
// test tk library
//
// tk use SDL3 callback system to organize program,
// and using internal variables to process graphics engine initialization,
// event handles and other.
//

#include "tk/tk.hpp"
#include "tk/log.hpp"

#include <chrono>

using namespace tk;
using namespace tk::ui;

class AudioPlayer
{
public:
  AudioPlayer()
  {
    // init tk context
    // and set title and extent of main window
    // and set user data
    init_tk_context("tk", 120, 30, this);

    // init background color
    init_background(Color::OneDark);

    // create layout and ui widgets
    _layout     = create_layout();
    _front_line = create_line(20, 3, 0.f, Color::Blue);
    _back_line  = create_line(100, 3, 0.f, Color::Grey);

    // set layout and ui widgets' position
    _layout->bind(get_main_window()).set_position(0, 0);
    _front_line->bind(_layout).set_position(10, 15).set_depth(0.11f);
    _back_line->bind(_layout).set_position(10, 15);
  }

  void set_playback_progress(uint32_t progress)
  {
    _front_line->set_length(progress);
  }

private:
  Layout* _layout;
  Line*   _front_line;
  Line*   _back_line;
};

void tk_init(int argc, char** argv)
{
  new AudioPlayer;
}

void tk_iterate()
{
  static auto start_time = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  auto progress = milliseconds % 10000 / 100;

  log::info("progress {}%", progress);
  
  auto audio_player = reinterpret_cast<AudioPlayer*>(get_user_data());
  audio_player->set_playback_progress(progress);
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
  delete reinterpret_cast<AudioPlayer*>(get_user_data());
}
