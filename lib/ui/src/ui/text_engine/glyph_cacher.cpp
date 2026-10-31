#include "glyph_cacher.hpp"
#include "../../util/file_system.hpp"
#include "../../util/async/thread_pool.hpp"
#include "../../util/compressor/compressor.hpp"

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
1. pregenerate glyph cache file
2. batch upload glyph cache before really use them
3. specific language
4. add grapheme clusters support
5. add entirely text api support, like line break, cursor move, selection, IME
*/
void GlyphCacher::destroy() noexcept
{
  assert(_file);
  if (!_glyphs.empty() || _file->size() == 0)
    g_thread_pool.submit([this, file = _file, glyphs = std::move(_glyphs)] mutable
    {
      auto data = serialize(file, glyphs);
      data = g_compressor.compression(data);
      err_if(!file->write(data, true), "failed to save glyph cache file: file I/O is still pending");
    });

  _glyphs.clear();
  _preload_glyphs.clear();
  _file = nullptr;
}

auto GlyphCacher::serialize(File* file, std::span<GlyphCacheData> glyphs) const noexcept -> std::vector<uint8>
{
  auto const needs_magic = file->size() == 0;
  auto const size = std::ranges::fold_left(glyphs, uint64{}, [](auto size, auto const& glyph)
    { return size + sizeof(glyph.header) + glyph.bitmap.size(); }) + (needs_magic ? sizeof(Magic) : 0);
  auto data = std::vector<uint8>(size);

  auto ptr = data.data();
  if (needs_magic)
  {
    memcpy(ptr, &Magic, sizeof(Magic));
    ptr += sizeof(Magic);
  }

  for (auto const& glyph : glyphs)
  {
    memcpy(ptr, &glyph.header, sizeof(glyph.header));
    ptr += sizeof(glyph.header);
    memcpy(ptr, glyph.bitmap.data(), glyph.bitmap.size());
    ptr += glyph.bitmap.size();
  }

  return data;
}

auto GlyphCacher::preload() noexcept -> Task<>
{
  assert(!_file);
  _file = co_await g_file_sys.open(Cache_Path);
  if (_file->size())
    co_await g_thread_pool.run([this]
    {
      auto data = g_compressor.decompression({ _file->data(), _file->size() });
      deserialize(data);
    });
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
