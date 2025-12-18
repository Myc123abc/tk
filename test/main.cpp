#include "tk/tk.hpp"

#include <thread>
#include <chrono>

int main()
{
  tk::init();

  tk::test();

  while (tk::window_count())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  tk::destroy();
}
