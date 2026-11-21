module;

#include <utility>

export module tk.ui:common;

import tk.hash;
import tk.string_id;

export namespace tk::ui {

/**
 * get frame delta time
 * @return delta time(us)
 */
auto delta_time() noexcept -> double;

/**
 * push a scope onto the id stack, every widget id generated before the matching pop_id
 * is hashed with this scope, so the same name can be reused in different scopes (e.g. loops)
 * must be called between window begin/end, and balanced before end
 * @param name scope name, can be a string, an integer or a hash value
 */
void push_id(StringID name) noexcept;

/**
 * push_id with multiple values, they are combined by tk::hash, e.g. push_id(name, i)
 */
template <typename First, typename Second, typename... Rest>
void push_id(First&& first, Second&& second, Rest&&... rest) noexcept
{
  push_id(StringID{ hash(std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...) });
}

/**
 * pop the scope pushed by push_id
 */
void pop_id() noexcept;

/**
 * RAII helper for push_id/pop_id, accepts the same params as push_id
 * e.g. IDScope{ "name" }, IDScope{ i }, IDScope{ name, i }
 */
struct IDScope
{
  // single value goes through StringID so string literals keep the consteval hash
  IDScope(StringID name) noexcept { push_id(name); }
  ~IDScope()             noexcept { pop_id();      }
  IDScope(IDScope const&)            = delete;
  IDScope(IDScope&&)                 = delete;
  IDScope& operator=(IDScope const&) = delete;
  IDScope& operator=(IDScope&&)      = delete;

  template <typename First, typename Second, typename... Rest>
  IDScope(First&& first, Second&& second, Rest&&... rest) noexcept
  {
    push_id(std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...);
  }
};

enum class CornerFlag
{
  top_left     = 1 << 0,
  top_right    = 1 << 1,
  bottom_left  = 1 << 2,
  bottom_right = 1 << 3,

  none   = 1 << 4,
  top    = top_left    | top_right,
  bottom = bottom_left | bottom_right,
  left   = top_left    | bottom_left,
  right  = top_right   | bottom_right,
  all    = top         | bottom,
};

}
