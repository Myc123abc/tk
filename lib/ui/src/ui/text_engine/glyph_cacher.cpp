#include "glyph_cacher.hpp"
#include "../../util/file_system.hpp"
#include "../../util/iocp.hpp"

#include <utility>

namespace tk::ui {

namespace {

constexpr auto Cache_Path = StringLiteral{ ".cache\\glyphs.bin" };
constexpr auto Magic      = static_cast<uint>('T') | static_cast<uint>('K') << 8 | static_cast<uint>('G') << 16 | static_cast<uint>('C') << 24;

}

void GlyphCacher::add(PendingCopyGlyphsInfoType& info) noexcept
{
  if (info.empty()) return;
  _glyphs.reserve(_glyphs.size() + std::ranges::distance(info | std::views::values | std::views::join));
  for (auto const& bitmap_infos : info | std::views::values)
  {
    for (auto const& [bitmap, _] : bitmap_infos)
    {
      _glyphs.emplace_back
      (
        GlyphCacheHeader
        {
          bitmap.glyph_key,
          bitmap.extent,
          bitmap.pos_offset,
          static_cast<uint>(bitmap.data.size()),
        },
        std::move(bitmap.data)
      );
    }
  }
}

/*
TODO:
unsave Skyline packer data, directly use a new texture when preloading
impl font pregenerate cache files
*/
void GlyphCacher::destroy() noexcept
{
  assert(_file);
  if (!_glyphs.empty() || _file->size() == 0)
    err_if(!g_file_sys.write(Cache_Path, serialize(), true),
      "failed to save glyph cache file: file I/O is still pending");

  _glyphs.clear();
  _preload_glyphs.clear();
  _file = nullptr;
}

auto GlyphCacher::serialize() const noexcept -> std::vector<uint8>
{
  auto const needs_magic = _file->size() == 0;
  auto const size = std::ranges::fold_left(_glyphs, uint64{}, [](auto size, auto const& glyph)
    { return size + sizeof(glyph.header) + glyph.bitmap.size(); }) + (needs_magic ? sizeof(Magic) : 0);
  auto data = std::vector<uint8>(size);

  auto ptr = data.data();
  if (needs_magic)
  {
    memcpy(ptr, &Magic, sizeof(Magic));
    ptr += sizeof(Magic);
  }

  for (auto const& glyph : _glyphs)
  {
    memcpy(ptr, &glyph.header, sizeof(glyph.header));
    ptr += sizeof(glyph.header);
    memcpy(ptr, glyph.bitmap.data(), glyph.bitmap.size());
    ptr += glyph.bitmap.size();
  }

  return data;
}

void GlyphCacher::preload() noexcept
{
  assert(!_file);

  auto opened = g_file_sys.write(Cache_Path, {}, true);
  err_if(!opened, "failed to open glyph cache file");

  // TODO: change to async link calls, and only wait point when need to use glyph cacher.
  auto result = g_file_sys.load(Cache_Path);
  while (!result)
  {
    err_if(!result.error().is<FileLoadError::loading>(), "failed to load glyph cache file");
    g_iocp.poll(INFINITE);
    result = g_file_sys.load(Cache_Path);
  }

  _file = *result;
  if (_file->size())
    deserialize({ _file->data(), static_cast<size_t>(_file->size()) });
}

void GlyphCacher::deserialize(std::span<uint8 const> data) noexcept
{
  assert(data.size() >= sizeof(Magic));
  err_if(memcmp(data.data(), &Magic, sizeof(Magic)) != 0,
    "Failed to deserialize glyph cache file, magic wrong");
  data = data.subspan(sizeof(Magic));
  
  while (!data.empty())
  {
    err_if(data.size() < sizeof(GlyphCacheHeader),
      "Failed to deserialize, glyph cache file incomplete");

    auto header = GlyphCacheHeader{};
    memcpy(&header, data.data(), sizeof(header));
    data = data.subspan(sizeof(header));

    err_if(header.bitmap_size > data.size(),
      "Failed to deserialize, glyph cache file bitmap information wrong");
    
    auto& glyph = _preload_glyphs[header.key];
    glyph.extent     = header.extent;
    glyph.pos_offset = header.pos_offset;
    glyph.bitmap.resize(header.bitmap_size);
    memcpy(glyph.bitmap.data(), data.data(), glyph.bitmap.size());

    data = data.subspan(glyph.bitmap.size());
  }
}

}
