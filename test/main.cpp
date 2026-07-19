#include "tk/tk.hpp"
#include "fps.hpp"
#include "playback_btn.hpp"

#include <string>
#include <span>
#include <format>

using namespace tk;

auto point_in_circle(float2 center, float radius, float theta) noexcept -> float2
{
  auto a = radians(theta);
  return { center.x + radius * std::cos(a), center.y + radius * std::sin(a) };
}

void circle_draw_test() noexcept
{
  auto [w, h] = ui::window_drawable_extent();
  auto center = float2{ 3, h - 3 };
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
  auto p0 = float2{}, p1 = float2{};
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

void test_path_draw() noexcept
{
  auto pos = float2{ 10, 10 };
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x + 50, pos.y + 75 });
  ui::path_end(true, 0xff0000ff, 0);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x + 50, pos.y + 75 });
  ui::path_end(true, 0xff0000ff, 1);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x + 50, pos.y + 75 });
  ui::path_end(false, 0xff0000ff, 1);

  pos = float2{ 10, 10 };
  pos.y += 110;
  ui::path_begin({ pos.x + 50, pos.y + 25 });
  ui::path_arc_to({ pos.x + 50, pos.y + 50 }, { pos.x + 25, pos.y + 50 }, false);
  ui::path_end(false, 0xff0000ff, 0);

  pos.x += 110;
  ui::path_begin({ pos.x + 15, pos.y + 85 });
  ui::path_line_to({ pos.x + 40, pos.y + 35 });
  ui::path_arc_to({ pos.x + 60, pos.y + 35 }, { pos.x + 80, pos.y + 35 }, false);
  ui::path_line_to({ pos.x + 105, pos.y + 85 });
  ui::path_end(false, 0xff0000ff, 3);

  pos.x += 110;
  ui::path_begin({ pos.x + 50, pos.y + 25 });
  ui::path_arc_to({ pos.x + 50, pos.y + 50 }, { pos.x + 25, pos.y + 50 }, false);
  ui::path_end(false, 0xff0000ff, 3);

  pos = float2{ 10, 10 };
  pos.y += 110 * 2;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_quad_bezier_to({ pos.x + 100, pos.y + 50 }, pos);
  ui::path_end(false, 0xff0000ff, 0);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_quad_bezier_to({ pos.x + 100, pos.y + 50 }, pos);
  ui::path_end(false, 0xff0000ff, 3);

  pos.x += 110;
  ui::path_begin({ pos.x, pos.y + 100 });
  ui::path_quad_bezier_to({ pos.x + 100, pos.y + 50 }, pos);
  ui::path_end(false, 0xff0000ff, 3);

  pos = float2{ 10, 10 };
  pos.y += 110 * 3;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y });
  ui::path_line_to({ pos.x + 50, pos.y + 40 });
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_end(true, 0xff0000ff, 0);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y });
  ui::path_line_to({ pos.x + 50, pos.y + 40 });
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_end(true, 0xff0000ff, 3);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y });
  ui::path_line_to({ pos.x + 50, pos.y + 40 });
  ui::path_line_to({ pos.x + 100, pos.y + 100 });
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_end(false, 0xff0000ff, 3);

  pos = float2{ 10, 10 };
  pos.y += 110 * 4;
  ui::path_begin(pos);
  ui::path_cubic_bezier_to({ pos.x + 50, pos.y }, { pos.x + 100, pos.y + 100 }, { pos.x + 50, pos.y + 100 });
  ui::path_end(true, 0xff0000ff, 0);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_cubic_bezier_to({ pos.x + 50, pos.y }, { pos.x + 100, pos.y + 100 }, { pos.x + 50, pos.y + 100 });
  ui::path_end(true, 0xff0000ff, 3);

  pos.x += 110;
  ui::path_begin(pos);
  ui::path_cubic_bezier_to({ pos.x + 50, pos.y }, { pos.x + 100, pos.y + 100 }, { pos.x + 50, pos.y + 100 });
  ui::path_end(false, 0xff0000ff, 3);

  pos = float2{ 10, 10 };
  pos.y += 110 * 5;
  ui::path_begin(pos);
  ui::path_line_to({ pos.x + 100, pos.y });
  ui::path_arc_to({ pos.x + 50, pos.y + 50 }, { pos.x + 100, pos.y + 100 }, false);
  ui::path_line_to({ pos.x, pos.y + 100 });
  ui::path_end(true, 0x00ffffff, 3);
}

inline auto img1 = "assets/image/test.jpg";
inline auto img2 = "assets/image/test.png";

void test_discard(uint fmt) noexcept
{
  auto _ = std::expected<void, ui::ImageLoadErrorType>{};
  ui::discard_beg([]{ ui::circle({ 50, 50 }, 25); });
  auto r = fmt % 7;
  switch (r)
  {
  case 0:
    ui::rectangle({}, { 100, 100 }, 0xffffffff);
    break;

  case 1:
    _ = ui::image(img1, {}, { 100, 100 }, 0x44);
    break;

  case 2:
    _ = ui::image(img1, {}, { 50, 50 });
    _ = ui::image(img1, { 50, 50 }, { 100, 100 });
    break;

  case 3:
    ui::rectangle({}, { 50, 50 }, 0xffffffff);
    _ = ui::image(img1, { 50, 50 }, { 100, 100 });
    break;

  case 4:
    _ = ui::image(img1, {}, { 50, 50 });
    ui::rectangle({ 50, 50 }, {}, 0xffffffff);
    break;

  case 5:
    ui::rectangle({}, { 50, 50 }, 0xffff00ff);
    _ = ui::image(img1, { 25, 25 }, { 70, 75 });
    ui::rectangle({ 50, 50 }, {}, 0xffffffff);
    break;

  case 6:
    _ = ui::image(img1, {}, { 50, 50 });
    ui::rectangle({ 25, 25 }, { 70, 75 }, 0xffff00ff);
    _ = ui::image(img1, { 50, 50 }, { 100, 100 });
    break;
  }
  ui::discard_end();
}

auto load_image(std::string_view path) noexcept
{
  if (auto res = ui::load_image(path); !res)
    res.error().visit(
      [&](ui::ImageLoadError::loading) { info("loading {}", path); },
      [&](ui::ImageLoadError::unexist) { warn("unexist image {}", path); },
      [&](ui::ImageLoadError::decode_failed const& err) { warn("decode failed of image {} : {}", path, err.msg); }
    );
}

auto image(std::string_view path, float2 left_top, float2 right_bottom, uint8 alpha = 0xff) noexcept -> std::expected<void, ui::ImageLoadErrorType>
{
  return ui::image(path, left_top, right_bottom, alpha).or_else([&](ui::ImageLoadErrorType err)
  {
    err.visit(
      [&](ui::ImageLoadError::loading) { info("loading {}", path); },
      [&](ui::ImageLoadError::unexist) { warn("unexist image {}", path); },
      [&](ui::ImageLoadError::decode_failed const& err) { warn("decode failed of image {} : {}", path, err.msg); }
    );
    return std::expected<void, ui::ImageLoadErrorType>{ std::unexpected(err) };
  });
}

auto fonts = std::vector<ui::FontInfo>{};

void load_font(std::string_view path) noexcept
{
  auto res = ui::load_font(path);
  if (!res.has_value())
  {
    res.error().visit(
      [&](ui::FontLoadError::unexist) { warn("failed to load font {}", path); },
      [&](ui::FontLoadError::freetype_err err) { warn("failed to load font {}, freetype error {}", path, err.code); }
    );
  }
  else
    fonts.emplace_back(res.value());
}

/*
TODO:
1. font-select text rendering
2. vertical text rendering
3. rotate rendering
*/

struct ButtonConfig
{
  std::string_view text;
  float            text_size{};
  ui::Color        text_color{};
  float4           padding{};

  ui::Color        button_color{};
  ui::Color        hover_color{};
  ui::Color        click_color{};
};

auto button(std::string_view name, float2 pos, float width, float height, ButtonConfig const& cfg) noexcept
{
  auto text_res = ui::text(cfg.text, cfg.text_size);
  auto res = button(name, pos.x, pos.y, width + cfg.padding.x + cfg.padding.z, (height ? height : text_res.extent.y) + cfg.padding.y + cfg.padding.w,
    cfg.button_color, cfg.hover_color, cfg.click_color);
  ui::text(cfg.text, pos + float2{ cfg.padding.x, cfg.padding.y }, cfg.text_size, cfg.text_color);
  return res;
}

struct SelectList_State
{
  float2 extent;
  int    idx{ -1 };
};

struct SelectListConfig
{
  float     size{};
  float     width{};
  ui::Color border_color;
  float4    padding{};

  ui::Color button_color;
  ui::Color hover_color;
  ui::Color click_color;
  ui::Color text_color;
};

auto select_list(std::string_view name, float2 pos, std::span<std::string_view> items, SelectListConfig const& cfg) noexcept
{
  auto id_name = "tk::ui::select_list::";

  // calc max width
  auto max_width = 0.f;
  for (auto i = 0; i < items.size(); ++i)
  {
    auto ext = ui::text(items[i], cfg.size).extent;
    if (!cfg.width)
      max_width = std::max(max_width, ext.x);
  }
  if (cfg.width) max_width = cfg.width;

  // render buttons
  auto pos_y = pos.y;
  for (auto i = 0; i < items.size(); ++i)
  {
    auto ext = ui::text(items[i], cfg.size).extent;
    auto btn_cfg = ButtonConfig
    {
      .text         = items[i],
      .text_size    = cfg.size,
      .text_color   = cfg.text_color,
      .padding      = cfg.padding,
      .button_color = cfg.button_color,
      .hover_color  = cfg.hover_color,
      .click_color  = cfg.click_color,
    };
    button(id_name + std::string(name) + std::to_string(i), { pos.x, pos_y }, max_width, 0, btn_cfg);
    pos_y += ext.y + cfg.padding.y + cfg.padding.w;
  }

  // render border
  if (cfg.border_color.a)
    ui::rectangle(pos, float2{ pos.x + max_width + cfg.padding.x + cfg.padding.z, pos_y }, cfg.border_color, 1);
}

void select_font() noexcept
{
  auto sl_cfg = SelectListConfig
  {
    .size         = 12,
    .width        = 140,
    .border_color = 0xe1e4e8ff,
    .padding      = { 8, 3, 0, 3 },

    .button_color = 0xf6f8faff,
    .hover_color  = 0xebf0f4ff,
    .click_color  = 0xe2e5e9ff,
    .text_color   = 0x000000ff,
  };
  auto items = std::vector<std::string_view>{
    "一覧",
    "により",
    "三回",
    "asd",
    "bveqw",
    "qwv",
    "cx",
    "boj",
    "0j12",
    "vqg",
    "12wd0j",
    "das0k",
    "120jd0l",
    "12",
    "1", "2", "3", "4", "5", "6", "8"
  };
  select_list("select_list", { 150, 150 }, items, sl_cfg);
  // ui::button("test_b", 10, 10, 100, 100, 0x00ff00ff, 0xffff00ff);
}

void silder(std::string_view name, float x, float y, float width, float height, float beg, float end, float& v) noexcept
{
  if (beg >= end && width > 0 && height > 0) return;

  auto id_name   = "tk::ui::silder";
  auto bar_name  = std::format("{}::{}::{}", id_name, name, "bar");
  auto knob_name = std::format("{}::{}::{}", id_name, name, "knob");
  
  v = std::clamp(v, beg, end);

  auto ratio    = (v - beg) / (end - beg);
  auto r        = height / 2;
  auto len      = width - 2 * r;
  auto center   = float2{ x + r + len * ratio, y + r };
  auto left_top = float2{ x + r, y + r / 2 };

  auto bar_color        = 0x808080ff;
  auto bar_hover_color  = 0xa0a0a0ff;
  auto knob_color       = 0xff00ffff;
  auto knob_hover_color = 0x0000ffff;

  auto bar_state  = ui::button(bar_name, x, y, width, height);
  auto knob_state = ui::button(name, center.x - r, center.y - r, r * 2, r * 2);

  if (bar_state.hovered)  bar_color  = bar_hover_color;
  if (knob_state.hovered) knob_color = knob_hover_color;

  if (knob_state.down || bar_state.down)
  {
    knob_color = knob_hover_color;
    auto cursor_x  = ui::get_cursor_pos_on_window().x;
    auto cur_ratio = std::clamp((cursor_x - x - r) / len, 0.f, 1.f);
    v = beg + cur_ratio * (end - beg);
  }

  ui::rectangle(left_top, { x + r + len, y + r / 2 + r }, bar_color);
  ui::circle(center, r, knob_color);
}

int main()
{
  tk::init();

  load_image(img1);
  load_image(img2);

  load_font("assets/font/NotoSansJP-Regular.ttf");
  // load_font("assets/font/NotoSansJP-Bold.ttf");
  // load_font("assets/font/YuGothR.ttc");
  // load_font("assets/font/NotoSansSC-Regular.ttf");
  // load_font("assets/font/SitkaVF-Italic.ttf");
  load_font("assets/font/SourceCodePro-Regular.ttf");

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

  auto cfg = ui::WindowConfig{};
  cfg.display_title_bar             = true;
	cfg.display_window_shadow         = true;
  cfg.display_wireframe_only_active = true;
  cfg.wireframe_color               = 0x7160e8ff;
  auto cfg2 = cfg;
  cfg.backdrop.default_blur();

  auto fps = Fps{};
  fps.init(240);
  fps.start();

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
      auto pos = float2{ extent.x / 2 - 50, extent.y - 50 };
      auto size = ui::window_drawable_extent();
      // ui::triangle({ size.x / 2, size.y * .1f }, size * .9f, { size.y * .1f, size.y * .9 }, 0x00ff0044, 10);

      static auto fmt = 0;

      if (ui::button("btn1", 0, 0, 100, 100, 0xffffffff, 0x00ff00ff))
        ++fmt;
      if (ui::button("btn2", 50, 50, 100, 100, 0xffffffff, 0x00ff00ff))
        wnd2_is_closed = !wnd2_is_closed;
      ui::add_move_invalid_area({}, { 150, 150 });

      // silder
      static float outline_width{};
      silder("silder_test", 0, 90, 100, 10, 0, 0.5, outline_width);
      ui::text(std::format("outline width : {}", outline_width), { 0, 90 }, 24, 0x00ff00ff);

      auto cfg = ui::TextConfig{};
      cfg.outer_color = 0xff0000ff;
      cfg.outline_width = outline_width;
      auto res = ui::text("a c一覧", { 0, 0 }, 32, 0x000000ff, cfg);
      ui::text("abc一覧", { 0, res.extent.y }, 32, 0x000000ff);
      // ui::line({ 0, res.ascender }, { res.extent.x, res.ascender }, 0xff0000ff);

      // circle_draw_test();
      // line_draw_test();
      // test_path_draw();
      // test_discard(fmt);

      // ui::discard_beg([]{ ui::rectangle({70, 70}, {300, 90});});
      // ui::union_beg();
      // ui::circle({50, 50}, 40);
      // ui::circle({90, 90}, 40);
      // ui::union_end(0xff0000ff, 3);
      // ui::discard_end();

      ui::end();
    }

    if (!wnd2_is_closed)
    {
      ui::begin("wnd2", 0, 1080, 200, 200, &wnd2_is_closed, cfg2);

      auto wnd_ext = ui::window_drawable_extent();
      ui::rectangle({}, wnd_ext, 0x282c34ff);

      if (ui::get_key(ui::Key::Q)) wnd2_is_closed = true;

      // playback button
      auto p0 = float2{ 5, 5 };
      auto p1 = p0 + float2{ 12.5 * 1.414, 12.5 };
      auto p2 = p0 + float2{ 0, 25 };
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
      auto p = p1 + float2{ 5, 0 };
      auto progress = progress_tween.get() * 100;
      ui::rectangle(p, p + float2{ 100,      3 }, 0x808080ff);
      ui::rectangle(p, p + float2{ progress, 3 }, 0x0000ffff);

      // image
      auto res = std::expected<void, ui::ImageLoadErrorType>{};
      if (loop_trigger)
      {
        auto img_ext = ui::image_extent(img2);
        auto ext = wnd_ext - p2;
        auto scale = std::max(img_ext.x / ext.x, img_ext.y / ext.y);
        img_ext /= scale;
        res = image(img2, p2, p2 + img_ext);
      }
      if (res) loop_trigger.update();

      static auto blur_img_2 = false;

      ui::discard_beg([]{ ui::circle({ 50, 50 }, 50); });
      if (blur_img_2)
        res = image(img1, {}, wnd_ext, 0x44, ui::ImageConfig::blur(5, 5));
      else
        res = image(img1, {}, wnd_ext, 0x44);
      ui::rectangle({ 50, 50 }, { 100, 100 }, 0x00ff00ff);
      ui::discard_end();

      // circle point
      auto size = ui::window_drawable_extent();
      ui::circle(point_in_circle({ size.x - 30, size.y - 30}, 20, circle_lerplocator.get() * 360), 3, 0xffffffff);
      circle_lerplocator.update();

      auto text_cfg = ui::TextConfig{};

      auto unit = float2{ 10, 10 };
      auto bp_y = 50;
      ui::line({ 0, bp_y }, { wnd_ext.x, bp_y }, 0x00ff00ff);
      for (auto x = 0, i = 0; x < wnd_ext.x; x += unit.x, ++i)
      {
        auto p = float2{ x, bp_y - unit.y };
        ui::line({ x, bp_y }, p, 0x00ff00ff);

        if (i % 2 == 0)
        {
          text_cfg.pos_as_baseline = true;
          auto res = ui::text(std::to_string(i), 12, text_cfg);
          ui::text(std::to_string(i), { p.x - res.extent.x / 2, bp_y + res.extent.y }, 12, 0x00ff00ff, text_cfg);
        }
      }
      text_cfg.pos_as_baseline = {};

      auto text_pos = p2 + float2{ 0, 10 };
      auto text_res = ui::text("Hello, World!", 32, text_cfg);
      // auto text_res = ui::text("Hello, World!", text_pos, 32, 0xffff00ff, text_cfg);
      // ui::rectangle(text_pos, text_pos + text_res.extent, 0x00ff00ff, 1);
      // ui::line({ text_pos.x, text_pos.y + text_res.ascender }, { text_pos.x + text_res.extent.x, text_pos.y + text_res.ascender }, 0xffff00ff);
      text_pos.y += text_res.extent.y;

      text_res = ui::text("你好，世界！", 32, text_cfg);
      // ui::rectangle(text_pos, text_pos + text_res.extent, 0x00ff00ff, 1);
      // ui::line({ text_pos.x, text_pos.y + text_res.ascender }, { text_pos.x + text_res.extent.x, text_pos.y + text_res.ascender }, 0xffff00ff);
      text_pos.y += text_res.extent.y;

      text_res = ui::text("こんにちは、世界！", 32, text_cfg);
      // ui::rectangle(text_pos, text_pos + text_res.extent, 0x00ff00ff, 1);
      // ui::line({ text_pos.x, text_pos.y + text_res.ascender }, { text_pos.x + text_res.extent.x, text_pos.y + text_res.ascender }, 0xffff00ff);
      text_pos.y += text_res.extent.y;

      ui::line({ text_pos.x + 30, 0 }, { text_pos.x + 30, wnd_ext.y }, 0xff0000ff);
      ui::line({ text_pos.x + 80, 0 }, { text_pos.x + 80, wnd_ext.y }, 0xff0000ff);
      // ui::discard_beg([&] { ui::rectangle(text_pos + float2{ 30, -2 }, text_pos + float2{ 80, 50 }); });
      // text_cfg.family = fonts[1].family;
      text_cfg.outer_color = 0xff0000ff;
      text_res = ui::text("Hello 你好 こんにちは、世界！", text_pos, 32, 0xffffffff, text_cfg);
      ui::rectangle(text_pos, text_pos + text_res.extent, 0x00ff00ff, 1);
      ui::line({ text_pos.x, text_pos.y + text_res.ascender }, { text_pos.x + text_res.extent.x, text_pos.y + text_res.ascender }, 0xffff00ff);
      text_pos.y += text_res.extent.y;
      // ui::discard_end();

      if (ui::button("blur onoff", 100, 100, 50, 50, 0x0000ffff, 0x00ff00ff))
      {
        if (cfg.backdrop.style == ui::BackdropStyle::blur)
          cfg.backdrop.default_acrylic();
        else if (cfg.backdrop.style == ui::BackdropStyle::acrylic)
          cfg.backdrop.default_blur();
        blur_img_2 = !blur_img_2;
        load_font("assets/font/NotoSansJP-Regular.ttf");
      }

      select_font();

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
