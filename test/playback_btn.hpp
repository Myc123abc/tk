#pragma once

#include "tk/ui/ui.hpp"

using namespace tk;

class PlaybackButton
{
public:
  void init(std::string_view name) noexcept
  {
    _name = name;
    _lerp_name = std::string(name) + "lerp value";
  }

  auto operator()(float2 p0, float2 p1, float2 p2, ui::Color color, ui::Color hovered_color, float thickness) noexcept -> bool
  {
    auto width  = p1.x - p0.x;
    auto height = p2.y - p0.y;
    auto [clicked, hovered, _] = ui::button(_name, p0.x, p0.y, width, height);
    if (hovered) color = hovered_color;

    if (clicked) _paused = !_paused;
    auto v = ui::ping_pong(_lerp_name, !_paused, 100'000);

    _lerp_pts = std::vector<LerpPoint>
    {
      //         playback button                    pause button
      { p0,                                p0,                              },
      { p0 + float2(width / 2,  height / 4), p0 + float2( width / 3, 0)         },
      { p2 + float2(width / 2, -height / 4), p2 + float2( width / 3, 0)         },
      { p2,                                p2,                              },
      { p0 + float2(width / 2,  height / 4), { p1.x - width / 3, p0.y },      },
      { p1,                                { p1.x, p0.y },                  },
      { p1,                                { p1.x, p2.y },                  },
      { p2 + float2(width / 2, -height / 4), { p1.x - width / 3, p2.y },      },
      { p1,                                p0 + float2(width / 3, height / 2) },
      { { p0.x, p1.y },                    { p1.x - width / 3, p1.y },      },
    };

    _pts.clear();
    for (auto const& pt : _lerp_pts)
      _pts.emplace_back(ui::lerp(pt.p0, pt.p1, v));

    ui::union_beg();

    ui::path_begin(_pts[0]);
    ui::path_line_to(_pts[1]);
    ui::path_quad_bezier_to(_pts[8], _pts[2]);
    ui::path_line_to(_pts[3]);
    ui::path_end();

    ui::path_begin(_pts[7]);
    ui::path_quad_bezier_to(_pts[9], _pts[4]);
    ui::path_line_to(_pts[5]);
    if (_pts[6] != _pts[5])
      ui::path_line_to(_pts[6]);
    ui::path_end();

    ui::union_end(color, thickness);

    return clicked;
  }

  void pause() noexcept { _paused = true;  }
  void play()  noexcept { _paused = false; }

  void reset() noexcept
  {
    _paused = true;
    ui::reset_tween(_lerp_name);
  }

  auto is_paused() const noexcept { return _paused; }

private:
  struct LerpPoint
  {
    float2 p0{};
    float2 p1{};
  };

  std::string            _name;
  std::string            _lerp_name;
  std::vector<LerpPoint> _lerp_pts;
  std::vector<float2>    _pts{};
  bool                   _paused{ true };
};
