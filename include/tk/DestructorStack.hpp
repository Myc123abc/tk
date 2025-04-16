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

    DestructorStack(DestructorStack const&)            = delete;
    DestructorStack(DestructorStack&& ds)              { *this = std::move(ds); }
    DestructorStack& operator=(DestructorStack const&) = delete;
    DestructorStack& operator=(DestructorStack&& ds)
    {
      _destructors = std::move(ds._destructors);
      return *this;
    }

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
