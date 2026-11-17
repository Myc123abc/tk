module;

#include <cassert>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

module tk.ui.text.glyph_store;

import tk.async;
import tk.error_handling;
import tk.renderer;
import tk.ui.config;
import tk.ui.text.font_registry;
import tk.ui.text.glyph_cacher;

#define check(x, msg) err_if(static_cast<bool>(x), "[GlyphStore] {}", msg)

using namespace tk::renderer;

namespace tk::ui {

void GlyphStore::init() noexcept
{
  _current_glyph_atlas_idx = {};
  _packer.reset({ Glyph_Atlas_Width, Glyph_Atlas_Height });
  _glyph_atlas.emplace_back(
    g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));
}

void GlyphStore::destroy() noexcept
{
  for (auto handle : _glyph_atlas)
    g_img_mgr.destroy(handle);
  _glyph_atlas.clear();
  _glyph_infos.clear();
  _uncached_glyphs.clear();
  _ungenerated_glyphs.clear();
  _pending_copy_glyphs.clear();
  _pending_copy_preload_glyphs.clear();
  _generate_bitmap_tasks.clear();
}

auto GlyphStore::request(GlyphKey key) noexcept -> bool
{
  if (_glyph_infos.contains(key))
    return false;

  if (_uncached_glyphs.emplace(key).second)
    _ungenerated_glyphs.emplace(key);
  return true;
}

auto GlyphStore::calc_glyph_pos(float2 extent) noexcept -> std::pair<uint, float2>
{
  check(extent.x > Glyph_Atlas_Width || extent.y > Glyph_Atlas_Height,
        "too big glyph sdf bitmap, cannot be stored in glyph atlas");

  if (auto res = _packer.add(extent.x, extent.y); res)
    return { _current_glyph_atlas_idx, res.value() };

  ++_current_glyph_atlas_idx;
  _glyph_atlas.emplace_back(
    g_img_mgr.create(Glyph_Atlas_Width, Glyph_Atlas_Height, ImageFormat::rgba8_unorm, ImageType::srv));

  _packer.reset({ Glyph_Atlas_Width, Glyph_Atlas_Height });
  auto res = _packer.add(extent.x, extent.y);
  assert(res);
  return { _current_glyph_atlas_idx, res.value() };
}

void GlyphStore::submit_bitmap_generation_tasks(FontRegistry const& fonts) noexcept
{
  if (_ungenerated_glyphs.empty()) return;

  auto const task_limit = std::min(Glyph_Max_Concurrent_Generation_Tasks, g_thread_pool.size());
  while (!_ungenerated_glyphs.empty() && _generate_bitmap_tasks.size() < task_limit)
  {
    auto glyphs = std::vector<std::pair<Font*, GlyphKey>>{};
    glyphs.reserve(std::min(Glyphs_Per_Generation_Task, _ungenerated_glyphs.size()));
    for (auto it = _ungenerated_glyphs.begin();
         it != _ungenerated_glyphs.end() && glyphs.size() < Glyphs_Per_Generation_Task;)
    {
      glyphs.emplace_back(fonts.font(it->font_key), *it);
      it = _ungenerated_glyphs.erase(it);
    }

    _generate_bitmap_tasks.emplace_back(g_thread_pool.run([glyphs = std::move(glyphs)]
    {
      auto bitmaps = std::vector<MSDFBitmap>{};
      bitmaps.reserve(glyphs.size());
      for (auto const& [font, key] : glyphs)
        bitmaps.emplace_back(font->generate_bitmap(key.glyph_index, key));
      return bitmaps;
    }));
  }
}

auto GlyphStore::upload_bitmaps() noexcept -> bool
{
  if (_generate_bitmap_tasks.empty()) return false;

  for (auto it = _generate_bitmap_tasks.begin(); it < _generate_bitmap_tasks.end();)
  {
    if (it->is_completed())
    {
      for (auto& bitmap : it->take_result())
      {
        auto [glyph_atlas_idx, copy_pos] = calc_glyph_pos(bitmap.extent);
        _pending_copy_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), copy_pos);
      }
      it = _generate_bitmap_tasks.erase(it);
    }
    else
      ++it;
  }

  if (_pending_copy_glyphs.empty()) return false;
  upload_bitmaps(_pending_copy_glyphs);
  return true;
}

auto GlyphStore::upload_bitmaps_from_preload(FontRegistry const& fonts) noexcept -> bool
{
  auto& preload_glyphs = g_glyph_cacher.preload_glyphs();
  for (auto it = _ungenerated_glyphs.begin(); it != _ungenerated_glyphs.end();)
  {
    auto key  = *it;
    auto font = fonts.font(key.font_key);
    if (font->has_color_glyphs())
    {
      ++it;
      continue;
    }

    if (preload_glyphs.contains(key))
    {
      auto& glyph        = preload_glyphs[key];
      auto const visible = font->get_glyph_bounds(key.glyph_index);
      auto bitmap = MSDFBitmap{};
      bitmap.data           = std::move(glyph.bitmap);
      bitmap.extent         = glyph.extent;
      bitmap.visible_offset = visible.left_top() - glyph.pos_offset;
      bitmap.visible_extent = visible.extent();
      bitmap.pos_offset     = glyph.pos_offset;
      bitmap.glyph_key      = key;
      auto [glyph_atlas_idx, copy_pos] = calc_glyph_pos(bitmap.extent);
      _pending_copy_preload_glyphs[glyph_atlas_idx].emplace_back(std::move(bitmap), copy_pos);

      preload_glyphs.erase(key);
      it = _ungenerated_glyphs.erase(it);
    }
    else
      ++it;
  }

  upload_bitmaps(_pending_copy_preload_glyphs);
  return !_pending_copy_preload_glyphs.empty();
}

void GlyphStore::upload_bitmaps(PendingCopyGlyphsInfoType const& info) noexcept
{
  if (info.empty()) return;
  for (auto const& [glyph_atlas_idx, bitmap_infos] : info)
  {
    for (auto const& [bitmap, pos] : bitmap_infos)
    {
      auto res = _glyph_infos.emplace(bitmap.glyph_key, GlyphInfo{
        glyph_atlas_idx, pos, bitmap.extent, bitmap.pos_offset,
        bitmap.visible_offset, bitmap.visible_extent, bitmap.is_color });
      assert(res.second);
    }

    auto bitmap_copy_infos = bitmap_infos
      | std::views::filter([](auto const& bitmap_info) { return !bitmap_info.first.empty(); })
      | std::views::transform([](auto const& bitmap_info)
        { return BitmapCopyInfo{ bitmap_info.first.to_bitmap_view(), bitmap_info.second }; })
      | std::ranges::to<std::vector<BitmapCopyInfo>>();
    g_copy_engine.copy(std::move(bitmap_copy_infos), _glyph_atlas[glyph_atlas_idx]);
  }
}

auto GlyphStore::update(FontRegistry const& fonts) noexcept -> bool
{
  // Cached glyphs must be resolved before generation starts, otherwise startup
  // requests regenerate and append glyphs that are still being deserialized.
  if (!g_glyph_cacher.preload_complete())
    return false;

  auto have_bitmaps_upload = false;
  have_bitmaps_upload |= upload_bitmaps_from_preload(fonts);
  submit_bitmap_generation_tasks(fonts);
  have_bitmaps_upload |= upload_bitmaps();

  if (have_bitmaps_upload)
    for (auto it = _uncached_glyphs.begin(); it != _uncached_glyphs.end();)
      _glyph_infos.contains(*it) ? it = _uncached_glyphs.erase(it) : ++it;

  return have_bitmaps_upload;
}

void GlyphStore::clear_pending_copy_glyphs() noexcept
{
  g_glyph_cacher.store(_pending_copy_glyphs);
  _pending_copy_glyphs.clear();
  _pending_copy_preload_glyphs.clear();
}

}
