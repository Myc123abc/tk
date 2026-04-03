#include "tk/tk.hpp"

using namespace tk;

using Vec2 = glm::vec2;

auto point_on_circle(Vec2 center, float radius, float theta) noexcept -> Vec2
{
  auto a = glm::radians(theta);
  return { center.x + radius * std::cos(a), center.y + radius * std::sin(a) };
}

class PlaybackButton
{
public:
  void init(std::string_view name) noexcept
  {
    _name = name;
    _lerp_name = std::string(name) + "lerp value";
  }

  auto operator()(Vec2 p0, Vec2 p1, Vec2 p2, ui::Color color, ui::Color hovered_color, float thickness) noexcept -> bool
  {
    auto width  = p1.x - p0.x;
    auto height = p2.y - p0.y;
    auto [clicked, hovered, _] = ui::button(_name, p0.x, p0.y, width, height);
    if (hovered) color = hovered_color;

    if (clicked) _paused = !_paused;
    auto v = ui::lerp_ping_pong(_lerp_name, !_paused, 100'000);

    _lerp_pts = std::vector<LerpPoint>
    {
      //         playback button                    pause button
      { p0,                                p0,                              },
      { p0 + Vec2(width / 2,  height / 4), p0 + Vec2( width / 3, 0)         },
      { p2 + Vec2(width / 2, -height / 4), p2 + Vec2( width / 3, 0)         },
      { p2,                                p2,                              },
      { p0 + Vec2(width / 2,  height / 4), { p1.x - width / 3, p0.y },      },
      { p1,                                { p1.x, p0.y },                  },
      { p1,                                { p1.x, p2.y },                  },
      { p2 + Vec2(width / 2, -height / 4), { p1.x - width / 3, p2.y },      },
      { p1,                                p0 + Vec2(width / 3, height / 2) },
      { { p0.x, p1.y },                    { p1.x - width / 3, p1.y },      },
    };

    _pts.clear();
    for (auto const& pt : _lerp_pts)
      _pts.emplace_back(ui::lerp(pt.p0, pt.p1, v));

    auto change = _pts[1].x != _pts[8].x;

    ui::begin_union();

    ui::begin_path();
    ui::line(_pts[0], _pts[1]);
    if (change)
      ui::bezier(_pts[1], _pts[8], _pts[2]);
    else
      ui::line(_pts[1], _pts[2]);
    ui::line(_pts[2], _pts[3]);
    ui::line(_pts[3], _pts[0]);
    ui::end_path();

    ui::begin_path();
    ui::line(_pts[4], _pts[5]);
    ui::line(_pts[5], _pts[6]);
    ui::line(_pts[6], _pts[7]);
    if (change)
      ui::bezier(_pts[7], _pts[9], _pts[4]);
    else
      ui::line(_pts[7], _pts[4]);
    ui::end_path();

    ui::end_union(color, thickness);

    return clicked;
  }

  void pause() noexcept { _paused = true;  }
  void play()  noexcept { _paused = false; }

  void reset() noexcept
  {
    _paused = true;
    ui::reset_lerpolator(_lerp_name);
  }

  auto is_paused() const noexcept { return _paused; }

private:
  struct LerpPoint
  {
    Vec2 p0{};
    Vec2 p1{};
  };

  std::string            _name;
  std::string            _lerp_name;
  std::vector<LerpPoint> _lerp_pts;
  std::vector<Vec2>      _pts{};
  bool                   _paused{ true };
};

int main()
{
  tk::init();

  ui::load_font("assets/font/NotoSansJP-Regular.ttf");
  ui::load_font("assets/font/NotoSansSC-Regular.ttf");

  auto playback_btn = PlaybackButton{};
  playback_btn.init("playback button");

  auto progress_lerpolator = ui::Lerpolator{};
  progress_lerpolator.init(1'000'000);

  auto loop_trigger = ui::LoopTrigger{};
  loop_trigger.init(1'000'000, true);

  auto circle_lerplocator = ui::Lerpolator{};
  circle_lerplocator.init(250'000, ui::Lerpolator::Mode::loop);

  auto wnd1_is_closed = false;
  auto wnd2_is_closed = false;
  while (!wnd1_is_closed || !wnd2_is_closed)
  {
    auto cfg = ui::WindowConfig{};
    cfg.display_title_bar             = true;
		cfg.display_window_shadow         = true;
    cfg.display_wireframe_only_active = true;
    cfg.wireframe_color               = 0x7160e8ff;
    cfg.blur_backdrop                 = true;

    if (!wnd1_is_closed)
    {
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed, cfg);
      ui::rectangle({}, ui::window_drawable_extent(), 0x282c3444);
      if (ui::get_key(ui::Key::F11))
      {
        if (ui::is_fullscreen_window())
          ui::restore_fullscreen_window();
        else
          ui::fullscreen_window();
      }
      auto extent = ui::window_drawable_extent();
      auto pos = glm::vec2{ extent.x / 2 - 50, extent.y - 50 };
      ui::rectangle(pos, pos + glm::vec2(100), 0x000000ff);
      auto size = ui::window_drawable_extent();
      ui::triangle({ size.x / 2, 0 }, size, { 0, size.y }, 0x00ff004f, 10);
      if (ui::button("btn1", 0, 0, 100, 100, 0x00ff00ff, 0x0000ffff))
        circle_lerplocator.reverse();
      if (ui::button("btn2", 50, 50, 100, 100, 0x00ff00ff, 0x0000ffff))
        wnd2_is_closed = false;
      ui::add_move_invalid_area({}, { 150, 150 });

      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 0, 1080, 200, 200, &wnd2_is_closed, cfg);

      auto wnd_ext = ui::window_drawable_extent();
      ui::rectangle({}, wnd_ext, 0x282c34ff);

      if (ui::get_key(ui::Key::Q)) wnd2_is_closed = true;

      // playback button
      auto p0 = Vec2{ 5, 5 };
      auto p1 = p0 + Vec2{ 12.5 * 1.414, 12.5 };
      auto p2 = p0 + Vec2{ 0, 25 };
      if (playback_btn(p0, p1, p2, 0xffffffff, 0xdcdcdcff, 1))
        if (progress_lerpolator.is_not_started()) progress_lerpolator.start();

      if (!playback_btn.is_paused()) progress_lerpolator.update();
      if (progress_lerpolator.is_finished())
      {
        progress_lerpolator.reset();
        playback_btn.pause();
      }
      if (ui::get_key(ui::Key::Space))
      {
        if (playback_btn.is_paused())
        {
          playback_btn.play();
          if (progress_lerpolator.is_not_started())
            progress_lerpolator.start();
        }
        else
         playback_btn.pause();
      }

      // progress bar
      auto p = p1 + Vec2{ 5, 0 };
      auto progress = progress_lerpolator.get() * 100;
      ui::rectangle(p, p + Vec2{ 100,      3 }, 0x808080ff);
      ui::rectangle(p, p + Vec2{ progress, 3 }, 0x0000ffff);

      // image
      if (loop_trigger)
      {
        auto img_ext = ui::image_extent("assets/image/test.png");
        auto ext = wnd_ext - p2;
        auto scale = std::max(img_ext.x / ext.x, img_ext.y / ext.y);
        img_ext /= scale;
        // ui::image("assets/image/test.png", p2, p2 + img_ext);
      }
      loop_trigger.update();
      ui::image("assets/image/test.jpg", {}, wnd_ext, 0x44);

      // circle point
      auto size = ui::window_drawable_extent();
      ui::circle(point_on_circle({ size.x - 30, size.y - 30}, 20, circle_lerplocator.get() * 360), 3, 0xffffffff);
      circle_lerplocator.update();

      auto text_pos = p2 + Vec2{ 0, 10 };
      auto text_ext = ui::text("Hello, World!", text_pos, 32, 0xffff00ff);
      ui::rectangle(text_pos, text_ext, 0x00ff00ff, 1);

      ui::end();
    }

    ui::render();
  }

  tk::destroy();
}
