#include "tk/tk.hpp"

using namespace tk;

auto point_on_circle(glm::vec2 center, float radius, float theta) noexcept -> glm::vec2
{
  auto a = glm::radians(theta);
  return { center.x + radius * std::cos(a), center.y + radius * std::sin(a) };
}

struct LerpPoint
{
  glm::vec2 p0{};
  glm::vec2 p1{};
};

auto playback_btn(int x, int y, uint32_t width, uint32_t height, ui::Color color, ui::Color hovered_color, float thickness) noexcept -> bool
{
  auto b = ui::button("playback btn", x, y, width, height, {}, {});

  static auto res = false;
  if (b) res = !res;
  auto v = ui::lerp_ping_pong("playback btn lerp value", res, 1'000'000);

  auto p0 = glm::vec2{ x, y };
  auto p1 = glm::vec2{ x + width, y + height / 2 };
  auto p2 = glm::vec2{ x, y + height };

  auto lerp_pts = std::vector<LerpPoint>
  {
    // pause button                                           // playback buttin
    { p0,                                                    p0,                                    },
    { p0 + glm::vec2( width / 3, 0),                         p0 + glm::vec2(width / 2,  height / 4) },
    { p2 + glm::vec2( width / 3, 0),                         p2 + glm::vec2(width / 2, -height / 4) },
    { p2,                                                    p2,                                    },
    { glm::vec2(p1.x - static_cast<float>(width) / 3, p0.y), p0 + glm::vec2(width / 2,  height / 4) },
    { glm::vec2(p1.x, p0.y),                                 p1,                                    },
    { glm::vec2(p1.x, p2.y),                                 p1,                                    },
    { glm::vec2(p1.x - static_cast<float>(width) / 3, p2.y), p2 + glm::vec2(width / 2, -height / 4) },
    { p0 + glm::vec2(width / 3, height / 2),                 p1,                                    },
    { glm::vec2(p1.x - static_cast<float>(width) / 3, p1.y), glm::vec2(p0.x, p1.y),                 },
  };

  auto pts = std::vector<glm::vec2>{};
  pts.reserve(lerp_pts.size());
  for (auto const& pt : lerp_pts)
    pts.emplace_back(ui::lerp(pt.p0, pt.p1, v));

  ui::begin_union();

  ui::begin_path();
  ui::line(pts[0], pts[1]);
  // if (change)
    ui::bezier(pts[1], pts[8], pts[2]);
  // else
  //   ui::line(pts[1], pts[2]);
  ui::line(pts[2], pts[3]);
  ui::line(pts[3], pts[0]);
  ui::end_path();
  
  ui::begin_path();
  ui::line(pts[4], pts[5]);
  ui::line(pts[5], pts[6]);
  ui::line(pts[6], pts[7]);
  // if (change)
    ui::bezier(pts[7], pts[9], pts[4]);
  // else
  //   ui::line(pts[7], pts[4]);
  ui::end_path();

  ui::end_union(color, thickness);

  return b;
}

int main()
{
  tk::init();

  auto wnd1_is_closed = false;
  auto wnd2_is_closed = false;
  while (!wnd1_is_closed || !wnd2_is_closed)
  {
    auto cfg = ui::WindowConfig{};
    cfg.display_title_bar = true;

    if (!wnd1_is_closed)
    {
      ui::begin("wnd1", 50, 50, 200, 200, &wnd1_is_closed, cfg);
      // ui::rectangle({}, ui::window_extent(), 0xffffffff);
      auto size = ui::window_drawable_extent();
      ui::triangle({ size.x / 2, 0 }, size, { 0, size.y }, 0x00ff004f, 10);
      if (ui::button("btn1", 0, 0, 100, 100, 0x00ff00ff, 0x0000ffff))
        info("click 11");
      if (ui::button("btn2", 50, 50, 100, 100, 0x00ff00ff, 0x0000ffff))
        info("click 22");
      ui::add_move_invalid_area({}, { 150, 150 });
      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 100, 100, 200, 200, &wnd2_is_closed, cfg);
      ui::rectangle({}, ui::window_extent(), 0xffffffff);

      playback_btn(5, 5, 22.67499992, 30, 0x00ff00ff, 0x00ffffff, 1);

      // ui::begin_union();

      // ui::begin_path();
      // ui::line({ 0, 50 }, { 50, 0 });
      // ui::bezier({ 50, 0 }, { 50, 50 }, { 100, 100 });
      // ui::line({ 100, 100 }, { 0, 50 });
      // ui::end_path();

      // ui::circle({ 50, 50 }, ui::lerp_ping_pong("circle radius", 0, 50, 3'000));
      // ui::circle({ 50, 50 }, 40);

      // ui::end_union(0x000000ff, 3);

      // circle point
      static auto dur  = 0;
      static auto time = 250;
      static auto target_value = 360;
      
      dur = (dur + static_cast<int>(ui::delta_time() / 1000)) % time; // to ms
      auto theta = static_cast<double>(dur) / time * target_value;

      auto size = ui::window_drawable_extent();
      ui::circle(point_on_circle({ size.x - 60, size.y - 60}, 50, theta), 3, 0x000000ff);

      ui::end();
    }

    ui::render();
  }

  tk::destroy();
}
