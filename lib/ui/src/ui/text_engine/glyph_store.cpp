module;

#include <cassert>
#include <iterator>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

module tk.ui.text.glyph_store;

import tk.async;
import tk.error_handling;
import tk.renderer;
import tk.ui.text.config;
import tk.ui.text.font_registry;
import tk.ui.text.glyph_cacher;

#define check(x, msg) err_if(static_cast<bool>(x), "[GlyphStore] {}", msg)

using namespace tk::renderer;

namespace tk::ui {

namespace {

constexpr auto Cache_Completion_Batch_Size             = 256uz;
constexpr auto Cache_Completion_Max_Concurrent_Tasks   = 4uz;
constexpr auto Glyph_Max_Concurrent_Generation_Tasks   = 2uz;
constexpr auto Glyphs_Per_Generation_Task              = 4uz;

}

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
  _unuploaded_glyphs.clear();
  _ungenerated_glyphs.clear();
  _cache_ungenerated_glyphs.clear();
  _cache_generating_glyphs.clear();
  _pending_copy_glyphs.clear();
  _pending_copy_preload_glyphs.clear();
  _generate_bitmap_tasks.clear();
  _generate_cache_task.reset();
}

auto GlyphStore::emplace(GlyphKey key) noexcept -> bool
{
  if (_glyph_infos.contains(key))
    return false;

  if (_unuploaded_glyphs.emplace(key).second && !_cache_generating_glyphs.contains(key))
  {
    _cache_ungenerated_glyphs.erase(key);
    _ungenerated_glyphs.emplace(key);
  }
  return true;
}

void GlyphStore::add_glyph_cache(GlyphKey key) noexcept
{
  if (_glyph_infos.contains(key) || _unuploaded_glyphs.contains(key) || _cache_generating_glyphs.contains(key))
    return;
  _cache_ungenerated_glyphs.emplace(key);
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
  auto active_task_count = _generate_bitmap_tasks.size() + static_cast<size_t>(_generate_cache_task.has_value());
  while (!_ungenerated_glyphs.empty() && active_task_count < task_limit)
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
    ++active_task_count;
  }
}

void GlyphStore::submit_cache_generation_task(FontRegistry const& fonts) noexcept
{
  if (_generate_cache_task || _cache_ungenerated_glyphs.empty() || !_unuploaded_glyphs.empty() || !_generate_bitmap_tasks.empty())
    return;

  while (!_cache_ungenerated_glyphs.empty())
  {
    auto const it  = _cache_ungenerated_glyphs.begin();
    auto const key = *it;
    _cache_ungenerated_glyphs.erase(it);
    if (g_glyph_cacher.contains(key))
      continue;

    _cache_generating_glyphs.emplace(key);
    auto font = fonts.font(key.font_key);
    _generate_cache_task.emplace(g_thread_pool.run([font, key]
    {
      auto bitmaps = std::vector<MSDFBitmap>{};
      bitmaps.emplace_back(font->generate_bitmap(key.glyph_index, key));
      return bitmaps;
    }));
    return;
  }
}

auto GlyphStore::upload_bitmaps() noexcept -> bool
{
  auto generated = PendingCopyGlyphsInfoType{};

  for (auto it = _generate_bitmap_tasks.begin(); it < _generate_bitmap_tasks.end();)
  {
    if (it->is_completed())
    {
      for (auto& bitmap : it->take_result())
      {
        auto [glyph_atlas_idx, copy_pos] = calc_glyph_pos(bitmap.extent);
        generated[glyph_atlas_idx].emplace_back(std::move(bitmap), copy_pos);
      }
      it = _generate_bitmap_tasks.erase(it);
    }
    else
      ++it;
  }

  if (_generate_cache_task && _generate_cache_task->is_completed())
  {
    auto cache_only = std::vector<MSDFBitmap>{};
    for (auto& bitmap : _generate_cache_task->take_result())
    {
      _cache_generating_glyphs.erase(bitmap.glyph_key);
      if (_unuploaded_glyphs.contains(bitmap.glyph_key))
      {
        auto [glyph_atlas_idx, copy_pos] = calc_glyph_pos(bitmap.extent);
        generated[glyph_atlas_idx].emplace_back(std::move(bitmap), copy_pos);
      }
      else
        cache_only.emplace_back(std::move(bitmap));
    }
    _generate_cache_task.reset();
    g_glyph_cacher.store(cache_only);
  }

  if (generated.empty()) return false;
  upload_bitmaps(generated);
  retain_bitmaps(_pending_copy_glyphs, std::move(generated));
  return true;
}

auto GlyphStore::upload_bitmaps_from_preload(FontRegistry const& fonts) noexcept -> bool
{
  auto& preload_glyphs = g_glyph_cacher.preload_glyphs();
  auto generated = PendingCopyGlyphsInfoType{};
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
      generated[glyph_atlas_idx].emplace_back(std::move(bitmap), copy_pos);

      preload_glyphs.erase(key);
      it = _ungenerated_glyphs.erase(it);
    }
    else
      ++it;
  }

  if (generated.empty()) return false;
  upload_bitmaps(generated);
  retain_bitmaps(_pending_copy_preload_glyphs, std::move(generated));
  return true;
}

void GlyphStore::retain_bitmaps(PendingCopyGlyphsInfoType& destination, PendingCopyGlyphsInfoType&& source) noexcept
{
  for (auto& [atlas_idx, bitmaps] : source)
  {
    auto& pending = destination[atlas_idx];
    pending.insert(pending.end(), std::make_move_iterator(bitmaps.begin()), std::make_move_iterator(bitmaps.end()));
  }
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

  g_glyph_cacher.update();

  auto have_bitmaps_upload = false;
  have_bitmaps_upload |= upload_bitmaps_from_preload(fonts);
  have_bitmaps_upload |= upload_bitmaps();

  if (have_bitmaps_upload)
    for (auto it = _unuploaded_glyphs.begin(); it != _unuploaded_glyphs.end();)
      _glyph_infos.contains(*it) ? it = _unuploaded_glyphs.erase(it) : ++it;

  submit_bitmap_generation_tasks(fonts);
  submit_cache_generation_task(fonts);

  return have_bitmaps_upload;
}

void GlyphStore::complete_cache_generation(FontRegistry const& fonts) noexcept
{
  g_glyph_cacher.wait_preload();

  if (_generate_cache_task)
  {
    auto bitmaps = _generate_cache_task->take_result();
    _generate_cache_task.reset();
    _cache_generating_glyphs.clear();
    g_glyph_cacher.store(bitmaps);
  }

  while (!_cache_ungenerated_glyphs.empty())
  {
    auto glyphs = std::vector<std::pair<Font*, GlyphKey>>{};
    glyphs.reserve(std::min(Cache_Completion_Batch_Size, _cache_ungenerated_glyphs.size()));
    while (!_cache_ungenerated_glyphs.empty() && glyphs.size() < Cache_Completion_Batch_Size)
    {
      auto const it  = _cache_ungenerated_glyphs.begin();
      auto const key = *it;
      _cache_ungenerated_glyphs.erase(it);
      if (!g_glyph_cacher.contains(key))
        glyphs.emplace_back(fonts.font(key.font_key), key);
    }
    if (glyphs.empty()) continue;

    auto const task_count = std::min({Cache_Completion_Max_Concurrent_Tasks, g_thread_pool.size(), glyphs.size() });
    auto task_glyphs = std::vector<std::vector<std::pair<Font*, GlyphKey>>>(task_count);
    for (auto i = 0uz; i < glyphs.size(); ++i)
      task_glyphs[i % task_count].emplace_back(glyphs[i]);

    auto tasks = std::vector<Task<std::vector<MSDFBitmap>>>{};
    tasks.reserve(task_count);
    for (auto& batch : task_glyphs)
    {
      tasks.emplace_back(g_thread_pool.run([batch = std::move(batch)]
      {
        auto bitmaps = std::vector<MSDFBitmap>{};
        bitmaps.reserve(batch.size());
        for (auto const& [font, key] : batch)
          bitmaps.emplace_back(font->generate_bitmap(key.glyph_index, key));
        return bitmaps;
      }));
    }

    auto bitmaps = std::vector<MSDFBitmap>{};
    bitmaps.reserve(glyphs.size());
    for (auto& task : tasks)
    {
      auto generated = task.take_result();
      bitmaps.append_range(std::move(generated));
    }

    g_glyph_cacher.store(bitmaps);
    g_glyph_cacher.update();
  }
}

void GlyphStore::cancel_cache_generation() noexcept
{
  _cache_ungenerated_glyphs.clear();
  if (!_generate_cache_task) return;

  auto bitmaps = _generate_cache_task->take_result();
  _generate_cache_task.reset();
  _cache_generating_glyphs.clear();
  g_glyph_cacher.store(bitmaps);
}

void GlyphStore::clear_pending_copy_glyphs() noexcept
{
  g_glyph_cacher.store(_pending_copy_glyphs);
  _pending_copy_glyphs.clear();
  _pending_copy_preload_glyphs.clear();
}

}
