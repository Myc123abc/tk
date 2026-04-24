#include "tk/tk.hpp"

using namespace tk;

auto point_in_circle(vec2 center, float radius, float theta) noexcept -> vec2
{
  auto a = radians(theta);
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

  auto operator()(vec2 p0, vec2 p1, vec2 p2, ui::Color color, ui::Color hovered_color, float thickness) noexcept -> bool
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
      { p0 + vec2(width / 2,  height / 4), p0 + vec2( width / 3, 0)         },
      { p2 + vec2(width / 2, -height / 4), p2 + vec2( width / 3, 0)         },
      { p2,                                p2,                              },
      { p0 + vec2(width / 2,  height / 4), { p1.x - width / 3, p0.y },      },
      { p1,                                { p1.x, p0.y },                  },
      { p1,                                { p1.x, p2.y },                  },
      { p2 + vec2(width / 2, -height / 4), { p1.x - width / 3, p2.y },      },
      { p1,                                p0 + vec2(width / 3, height / 2) },
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
    ui::reset_tween(_lerp_name);
  }

  auto is_paused() const noexcept { return _paused; }

private:
  struct LerpPoint
  {
    vec2 p0{};
    vec2 p1{};
  };

  std::string            _name;
  std::string            _lerp_name;
  std::vector<LerpPoint> _lerp_pts;
  std::vector<vec2>      _pts{};
  bool                   _paused{ true };
};

struct FrameRate
{
  float    deltas[60]{};
  uint32_t idx{};
  float    accum{};
  uint32_t cnt{};
  float    fps{};

  void update() noexcept
  {
    // delta unit change to sec
    auto delta = ui::delta_time() / 1000'000;

    // calc accum
    accum += delta - deltas[idx];

    // store delta
    deltas[idx] = delta;

    // move to next
    idx = (idx + 1) % _countof(deltas);

    // get delta cnt
    cnt = std::min(cnt + 1, static_cast<uint32_t>(_countof(deltas)));

    // calc fps
    fps = accum > 0.f ? 1.f / (accum / cnt) : std::numeric_limits<float>::max();
  }

  auto get() const noexcept { return fps; }

} fps;

void circle_draw_test() noexcept
{
  auto [w, h] = ui::window_drawable_extent();
  auto center = vec2{ 3, h - 3 };
  auto radius = 1.f;
  while (center.x + radius + 2 < w)
  {
    ui::circle(center, radius, 0x00ff00ff);
    auto old_radius = radius;
    ++radius;
    center.x += old_radius + 2 + radius;
    center.y  = h - 2 - radius;
  }
}

void line_draw_test() noexcept
{
  auto [w, h] = ui::window_drawable_extent();
  auto p0 = vec2{}, p1 = vec2{};
  auto t = 0.f;
  while (p1.x < w && p0.y < h)
  {
    auto old_t = t;
    ++t;
    p1.x += old_t + t + 2;
    p0.y += old_t + t + 2;
    ui::line(p0, p1, 0x00ff00ff, t);
  }
  while (p1.y < h && p0.x < w)
  {
    auto old_t = t;
    --t;
    p1.y += old_t + t + 2;
    p0.x += old_t + t + 2;
    ui::line(p0, p1, 0x00ff00ff, t);
  }
}

int main()
{
  tk::init();

  auto img1 = "assets/image/test.jpg";
  auto img2 = "assets/image/test.png";
  ui::load_image(img1);
  ui::load_image(img2);

  ui::load_font("assets/font/NotoSansJP-Regular.ttf");
  ui::load_font("assets/font/NotoSansSC-Regular.ttf");

  auto playback_btn = PlaybackButton{};
  playback_btn.init("playback button");

  auto progress_tween = ui::Tween{};
  progress_tween.init(1'000'000);

  auto loop_trigger = ui::LoopTrigger{};
  loop_trigger.init(1'000'000, true);

  auto circle_lerplocator = ui::Tween{};
  circle_lerplocator.init(250'000, ui::Tween::Mode::loop);

  auto wnd1_is_closed = false;
  auto wnd2_is_closed = true;

  auto is_loaded = false;

  auto cfg = ui::WindowConfig{};
  cfg.display_title_bar             = true;
	cfg.display_window_shadow         = true;
  cfg.display_wireframe_only_active = true;
  cfg.wireframe_color               = 0x7160e8ff;
  auto cfg2 = cfg;
  cfg.backdrop.default_blur();

  while (!wnd1_is_closed || !wnd2_is_closed)
  {
    if (!wnd1_is_closed)
    {
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed, cfg);
      if (ui::get_key(ui::Key::F11))
      {
        if (ui::is_fullscreen_window())
          ui::restore_fullscreen_window();
        else
          ui::fullscreen_window();
      }
      auto extent = ui::window_drawable_extent();
      auto pos = vec2{ extent.x / 2 - 50, extent.y - 50 };
      auto size = ui::window_drawable_extent();
      ui::triangle({ size.x / 2, size.y * .1f }, size * .9f, { size.y * .1f, size.y * .9 }, 0x00ff0044, 10);

      if (ui::button("btn1", 0, 0, 100, 100, 0xffffffff, 0xffffffff))
        circle_lerplocator.reverse();
      if (ui::button("btn2", 50, 50, 100, 100, 0xffffffff, 0xffffffff))
        wnd2_is_closed = !wnd2_is_closed;
      ui::add_move_invalid_area({}, { 150, 150 });

      // circle_draw_test();
      // line_draw_test();

      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 0, 1080, 200, 200, &wnd2_is_closed, cfg2);

      auto wnd_ext = ui::window_drawable_extent();
      ui::rectangle({}, wnd_ext, 0x282c34ff);

      if (ui::get_key(ui::Key::Q)) wnd2_is_closed = true;

      // playback button
      auto p0 = vec2{ 5, 5 };
      auto p1 = p0 + vec2{ 12.5 * 1.414, 12.5 };
      auto p2 = p0 + vec2{ 0, 25 };
      if (playback_btn(p0, p1, p2, 0xffffffff, 0xdcdcdcff, 1))
        if (progress_tween.is_not_started()) progress_tween.start();

      if (!playback_btn.is_paused()) progress_tween.update();
      if (progress_tween.is_finished())
      {
        progress_tween.reset();
        playback_btn.pause();
      }
      if (ui::get_key(ui::Key::Space))
      {
        if (playback_btn.is_paused())
        {
          playback_btn.play();
          if (progress_tween.is_not_started())
            progress_tween.start();
        }
        else
         playback_btn.pause();
      }

      // progress bar
      auto p = p1 + vec2{ 5, 0 };
      auto progress = progress_tween.get() * 100;
      ui::rectangle(p, p + vec2{ 100,      3 }, 0x808080ff);
      ui::rectangle(p, p + vec2{ progress, 3 }, 0x0000ffff);

      // image
      if (loop_trigger)
      {
        auto img_ext = ui::image_extent(img2);
        auto ext = wnd_ext - p2;
        auto scale = std::max(img_ext.x / ext.x, img_ext.y / ext.y);
        img_ext /= scale;
        if (is_loaded = ui::image(img2, p2, p2 + img_ext); !is_loaded)
          info("loading {}", img2);
      }
      if (is_loaded) loop_trigger.update();

      if (!ui::image(img1, {}, wnd_ext, 0x44))
        info("loading {}", img1);

      // circle point
      auto size = ui::window_drawable_extent();
      ui::circle(point_in_circle({ size.x - 30, size.y - 30}, 20, circle_lerplocator.get() * 360), 3, 0xffffffff);
      circle_lerplocator.update();

      auto text_pos = p2 + vec2{ 0, 10 };
      auto text_ext = ui::text("Hello, World!", text_pos, 32, 0xffff00ff);
      ui::rectangle(text_pos, text_ext, 0x00ff00ff, 1);

      if (ui::button("blur onoff", 50, 50, 50, 50, 0x0000ffff, 0x00ff00ff))
      {
        if (cfg.backdrop.style == ui::BackdropStyle::blur)
          cfg.backdrop.default_acrylic();
        else if (cfg.backdrop.style == ui::BackdropStyle::acrylic)
          cfg.backdrop.default_blur();
      }

      ui::end();
    }

    tk::update();

    // fps
    static auto acc_time = 0;
    acc_time += ui::delta_time();
    if (acc_time >= 1000'000)
    {
      info("fps {}", fps.get());
      acc_time = 0;
    }
    fps.update();
  }

  tk::destroy();
}
