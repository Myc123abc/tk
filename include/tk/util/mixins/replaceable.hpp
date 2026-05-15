#pragma once

namespace tk {

struct Replaceable
{
  template <typename Self>
  auto replace(this Self& self, Self const& obj) noexcept
  {
    if (obj != self)
    {
      self = obj;
      return true;
    }
    return false;
  }
};

}
