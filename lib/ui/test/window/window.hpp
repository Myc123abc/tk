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

  BaseWindow(StringID name, std::optional<float2> ext, std::optional<float2> pos = {}, ui::WindowConfig const& cfg = {}) noexcept
    : _name(name), _pos(pos), _ext(ext), _cfg(cfg) {}

  void operator()() noexcept
  {
    if (!_is_shown)
    {
      _show_cnt = {};
      return;
    }

    auto is_closed = false;
    ui::begin(_name, _ext, _pos, &is_closed, _cfg);
    _is_shown = !is_closed;

    auto is_maximized  = ui::is_maximized_window();
    auto is_fullscreen = ui::is_fullscreen_window();
    if (!is_maximized && !is_fullscreen)
    {
      _pos = ui::window_pos();
      _ext = ui::window_extent();
    }

    if (_show_cnt == 0)
    {
      if (_is_maximized)  ui::maximize_window();
      if (_is_fullscreen) ui::fullscreen_window();
    }
    _is_maximized  = is_maximized;
    _is_fullscreen = is_fullscreen;

    static_cast<Derived*>(this)->draw();

    ++_show_cnt;
    ui::end();
  }

  void display_switch() noexcept { _is_shown = !_is_shown; }

  auto& is_shown() noexcept { return _is_shown; }

  void show() noexcept { _is_shown = true; }
  void hide() noexcept { _is_shown = false; }

private:
  StringID              _name;
  std::optional<float2> _pos;
  std::optional<float2> _ext;
  bool                  _is_shown{};
  ui::WindowConfig      _cfg;
  bool                  _is_maximized{};
  bool                  _is_fullscreen{};
  uint64                _show_cnt{};
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
