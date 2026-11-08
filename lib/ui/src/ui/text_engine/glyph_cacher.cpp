#include "glyph_cacher.hpp"
#include "ideographs.hpp"
#include "../../util/file_system.hpp"
#include "../../util/async/thread_pool.hpp"
#include "../../util/compressor/compressor.hpp"
#include "../../util/unicode.hpp"
#include "../../util/binary_file_format.hpp"

namespace tk::ui {

namespace {

constexpr auto Cache_Path = StringLiteral{ ".cache\\glyphs.bin" };
constexpr auto Magic_Text = StringID{ "tk::ui::glyph_cacher::glyph_cache_file" };

struct CommonGlyphInfoRecord
{
  uint64 font_family{};
  uint   language{};
  uint   font_style{};
  uint   direction{};
};

void append_range(std::string& text, uint first, uint last) noexcept
{
  for (auto codepoint = first; codepoint <= last; ++codepoint)
    append_codepoint(text, codepoint);
}

auto common_glyph_text(language lang) noexcept -> std::string
{
  auto text = std::string{};
  append_range(text, 0x0020, 0x00ff);

  auto ideographs = std::string_view{};
  switch (lang)
  {
  case language::en:
    break;
  case language::ch:
    append_range(text, 0x2000, 0x206f);
    append_range(text, 0x3000, 0x30ff);
    append_range(text, 0x31f0, 0x31ff);
    append_range(text, 0xff00, 0xffef);
    append_codepoint(text, 0xfffd);
    ideographs = Chinese_Common_Ideographs;
    break;
  case language::jp:
    append_range(text, 0x3000, 0x30ff);
    append_range(text, 0x31f0, 0x31ff);
    append_range(text, 0xff00, 0xffef);
    append_codepoint(text, 0xfffd);
    ideographs = Japanese_Common_Ideographs;
    break;
  default:
    std::unreachable();
  }

  text += ideographs;
  return text;
}

}

/*
TODO:
1. font glyphs.bin generation need
2. batch upload glyph cache before really use them
3. specific language
4. add entirely text api support, like line break, cursor move, selection, IME
*/
void GlyphCacher::init() noexcept
{
  _preload_task = preload();
}

auto GlyphCacher::preload() noexcept -> Task<>
{
  _file = co_await g_file_sys.open(Cache_Path);
  if (_file->size())
    co_await g_thread_pool.run([this]
    {
      auto data = g_compressor.decompression({ _file->data(), _file->size() });
      deserialize(data);
    });
}

void GlyphCacher::deserialize(std::span<uint8> data) noexcept
{
  auto file_fmt = BinaryFileFormat{};
  file_fmt.set_magic(Magic_Text);
  file_fmt.read(data);
  file_fmt.read_magic();
  while (!file_fmt.is_read_data_empty())
  {
    auto [header, data] = file_fmt.read_field<GlyphCacheHeader>();
    _preload_glyphs[header.key] = { header.extent, header.pos_offset, data | std::ranges::to<std::vector<uint8>>() };
  }
}

void GlyphCacher::destroy() noexcept
{
  write_cache();
}

void serialize(BinaryFileFormat& fmt, std::span<GlyphCacheField> glyphs, bool has_magic) noexcept
{
  if (!has_magic) fmt.set_magic(Magic_Text);
  for (auto const& [header, glyph] : glyphs)
    fmt.append(header, glyph);
  fmt.build();
}

void GlyphCacher::write_cache() noexcept
{
  if (_glyphs.empty()) return;
  g_thread_pool.submit([this, file = _file, glyphs = std::move(_glyphs)] mutable
  {
    auto file_fmt = BinaryFileFormat{};
    serialize(file_fmt, glyphs, _file->size());
    auto data = g_compressor.compression(file_fmt.data());
    err_if(!file->append(data), "failed to save glyph cache file: file I/O is still pending");
  });
}

void GlyphCacher::add(PendingCopyGlyphsInfoType& info) noexcept
{
  if (info.empty()) return;
  _glyphs.reserve(_glyphs.size() + std::ranges::distance(info | std::views::values | std::views::join));
  for (auto const& bitmap_infos : info | std::views::values)
    for (auto const& [bitmap, _] : bitmap_infos)
      _glyphs.emplace_back(GlyphCacheHeader{ bitmap.glyph_key, bitmap.extent, bitmap.pos_offset }, std::move(bitmap.data));
}

// auto GlyphCacher::pregenerate(std::span<PregenerateTextInfo const> infos) noexcept -> Task<>
// {
// }

// auto GlyphCacher::pregenerate(std::span<MSDFBitmap const> bitmaps) noexcept -> Task<>
// {
// }

}
