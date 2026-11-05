#pragma once

#include "tk/ui/ui.hpp"

using namespace tk;

template <typename Derived>
class BaseWindow
{
public:
  BaseWindow()                         = default;
  ~BaseWindow()                        = default;
  BaseWindow(BaseWindow const&)            = delete;
  BaseWindow(BaseWindow&&)                 = default;
  BaseWindow& operator=(BaseWindow const&) = delete;
  BaseWindow& operator=(BaseWindow&&)      = default;

  BaseWindow(StringID name, float2 ext, std::optional<float2> pos = {}, ui::WindowConfig const& cfg = {}) noexcept
    : _name(name), _pos(pos), _ext(ext), _cfg(cfg) {}

  void operator()() noexcept
  {
    if (!_is_shown) return;

    auto is_closed = false;
    ui::begin(_name, _ext.x, _ext.y, _pos, &is_closed, _cfg);
    _is_shown = !is_closed;

    static_cast<Derived*>(this)->draw();

    _pos = ui::window_pos();
    _ext = ui::window_drawable_extent();

    ui::end();
  }

  void display_switch() noexcept { _is_shown = !_is_shown; }

  auto& is_shown() noexcept { return _is_shown; }

  void show() noexcept { _is_shown = true; }
  void hide() noexcept { _is_shown = false;  }

private:
  StringID         _name;
  std::optional<float2> _pos;
  float2           _ext;
  bool             _is_shown{};
  ui::WindowConfig _cfg;
};

#define DefWindow(ClassName, ...)              \
class ClassName : public BaseWindow<ClassName> \
{                                              \
public:                                        \
  using BaseWindow<ClassName>::BaseWindow;     \
                                               \
  void draw() noexcept;                        \
                                               \
__VA_ARGS__                                    \
}
