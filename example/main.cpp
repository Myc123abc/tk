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

class AudioPlayer
{
public:
  AudioPlayer()
  {
    // init tk context
    // and set title and extent of main window
    // and set user data
    init_tk_context("tk", 200, 200, this);
  }

  void set_playback_progress(uint32_t progress)
  {
  }
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

  // log::info("progress {}%", progress);
  
  auto audio_player = reinterpret_cast<AudioPlayer*>(get_user_data());
  audio_player->set_playback_progress(progress);

  // test ui
  ui::begin(0, 0);
  ui::rectangle({ 0, 0 }, tk::get_main_window_extent(), 0xFF282C34);
  ui::rectangle({ 0, 0 }, { 100, 100 }, 0xFFFF0000);
  ui::rectangle({ 100, 0 }, { 100, 100 }, 0xFF00FF00);
  ui::rectangle({ 100, 100 }, { 100, 100 }, 0xFF0000FF);
  ui::rectangle({ 0, 100 }, { 100, 100 }, 0xFFFFFF00);
  ui::end();
}

void tk_event(SDL_Event* event)
{
}

void tk_quit()
{
  delete reinterpret_cast<AudioPlayer*>(get_user_data());
}
