#include "tk/audio/audio.hpp"
#include "tk/log.hpp"

#include <miniaudio.h>


using namespace tk;

namespace
{
  
void check(bool b, std::string_view msg)
{
  if (b) log::error("[audio] {}", msg);
}

}

namespace tk { namespace audio {

void play(std::string_view path)
{
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio engine.");
    }

    // TODO:
    // not use file name open by miniaudio because only utf8 with bom encode can used by miniaudio on windows
    // this is suck
    // change to directly load in memory
    ma_engine_play_sound(&engine, path.data(), NULL);

    ma_engine_uninit(&engine);
}

}}