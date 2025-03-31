//
// destructor stack
//
// use stack to save destructor objects
//

#pragma once

#include <stack>
#include <functional>

namespace tk
{

  class DestructorStack
  {
  public:
    DestructorStack()  = default;
    ~DestructorStack() = default;

    DestructorStack(DestructorStack const&)            = default;
    DestructorStack(DestructorStack&&)                 = delete;
    DestructorStack& operator=(DestructorStack const&) = delete;
    DestructorStack& operator=(DestructorStack&&)      = delete;

    void push(std::function<void()>&& func) { _destructors.push(func); }
    void clear()
    {
      while (!_destructors.empty())
      {
        _destructors.top()();
        _destructors.pop();
      }
    }

  private:
    std::stack<std::function<void()>> _destructors;
  };

}
