#pragma once

namespace tk { namespace audio {

class Audio
{
public:
  Audio()                        = default;
  Audio(Audio const&)            = delete;
  Audio(Audio&&)                 = delete;
  Audio& operator=(Audio const&) = delete;
  Audio& operator=(Audio&&)      = delete;

private:
};

}}