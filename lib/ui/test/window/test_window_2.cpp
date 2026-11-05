#include "test_window_2.hpp"
#include "../font.hpp"

auto point_in_circle(float2 center, float radius, float theta) noexcept -> float2
{
  auto a = radians(theta);
  return { center.x + radius * std::cos(a), center.y + radius * std::sin(a) };
}

struct ButtonConfig
{
  StringLiteral text;
  float         text_size{};
  ui::Color     text_color{};
  float4        padding{};

  ui::Color     button_color{};
  ui::Color     hover_color{};
  ui::Color     click_color{};
};

auto button(StringID name, float2 pos, float width, float height, ButtonConfig const& cfg) noexcept
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

auto select_list(StringLiteral name, float2 pos, std::span<StringLiteral> items, SelectListConfig const& cfg) noexcept
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
    button(id_name + std::string(name.view()) + std::to_string(i), { pos.x, pos_y }, max_width, 0, btn_cfg);
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
  auto items = std::vector<StringLiteral>{
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

void TestWindow2::draw() noexcept
{
  auto wnd_ext = ui::window_drawable_extent();
  ui::rectangle({}, wnd_ext, 0x282c34ff);

  if (ui::get_key(ui::Key::Q).is_down()) hide();

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
    res = image(img1, {}, wnd_ext, 0x44, ui::ImageConfig{{ 5, 5 }, 100, ui::CornerFlag::all});
    // res = image(img1, {}, wnd_ext, 0x44, ui::ImageConfig{{ 5, 5 }});
  else
    res = image(img1, {}, wnd_ext, 0x44);
  ui::rectangle({ 50, 50 }, { 100, 100 }, 0x00ff00ff, 0, 12);
  ui::rectangle({ 110, 50 }, { 160, 100 }, 0xff8800ff, 2, 12, ui::CornerFlag::top | ui::CornerFlag::bottom_right);
  ui::discard_end();

  // circle point
  auto size = ui::window_drawable_extent();
  ui::circle(point_in_circle({ size.x - 30, size.y - 30}, 20, circle_lerplocator.get() * 360), 3, 0xffffffff);
  circle_lerplocator.update();

  static auto text_cfg = ui::TextConfig{};

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
  text_cfg.outer_color = 0xff0000ff;
  text_res = ui::text("Hello 你好 こんにちは、世界！", text_pos, 32, 0xffffffff, text_cfg);
  ui::rectangle(text_pos, text_pos + text_res.extent, 0x00ff00ff, 1);
  ui::line({ text_pos.x, text_pos.y + text_res.ascender }, { text_pos.x + text_res.extent.x, text_pos.y + text_res.ascender }, 0xffff00ff);
  text_pos.y += text_res.extent.y;
  // ui::discard_end();

  if (ui::button("blur onoff", 100, 100, 50, 50, 0x0000ffff, 0x00ff00ff))
  {
    if (wnd_cfg.backdrop.style == ui::BackdropStyle::blur)
      wnd_cfg.backdrop = ui::Backdrop::Default_Acrylic();
    else if (wnd_cfg.backdrop.style == ui::BackdropStyle::acrylic)
      wnd_cfg.backdrop = ui::Backdrop::Default_Blur();
    blur_img_2 = !blur_img_2;

    for (auto path : fallback_font_paths)
      font_selector.add(path);
    font_selector.load_fonts();

    if (text_cfg.style == ui::FontStyle::regular)
      text_cfg.style = ui::FontStyle::bold;
    else
      text_cfg.style = ui::FontStyle::regular;
    text_cfg.family = font_selector.font_infos()[2].family;
  }

  select_font();

  // silder
  // static float outline_width{};
  // silder("silder_test", 0, 90, 100, 10, 0, 0.5, outline_width);
  // ui::text(std::format("outline width : {}", outline_width), { 0, 90 }, 24, 0x00ff00ff);
}
