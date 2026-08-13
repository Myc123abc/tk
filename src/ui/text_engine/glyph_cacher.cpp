#include "glyph_cacher.hpp"

#include <filesystem>

namespace tk::ui {

void GlyphCacher::add(PendingCopyGlyphsInfoType& info) noexcept
{
  if (info.empty()) return;
  _glyphs.reserve(_glyphs.size() + std::ranges::distance(info | std::views::values | std::views::join));
  for (auto const& bitmap_infos : info | std::views::values)
  {
    for (auto const& [bitmap, _] : bitmap_infos)
    {
      auto& item = _glyphs.emplace_back
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
      _cache_size += sizeof(item.header) + item.bitmap.size();
    }
  }
}

struct PendingWrite
{
  OVERLAPPED         overlapped;
  std::vector<uint8> data;
};
static PendingWrite pending_write;
static auto pending = false;

void GlyphCacher::try_save() noexcept
{
  if (pending)
  {
    DWORD bytes_written{};
    auto res = GetOverlappedResult(_file, &pending_write.overlapped, &bytes_written, FALSE);
    err_if(!res && GetLastError() != ERROR_IO_INCOMPLETE, "Failed to GetOverlappedResult");
    if (res) pending = false;
  }
  else if (_cache_size > Glyph_Local_Cache_Capacity)
  {
    pending = true;
    save();
  }
}

void GlyphCacher::block_save() noexcept
{
  save();
  DWORD bytes_written{};
  auto res = GetOverlappedResult(_file, &pending_write.overlapped, &bytes_written, FALSE);
  err_if(!res && GetLastError() != ERROR_IO_INCOMPLETE, "Failed to GetOverlappedResult");

  while (!res)
  {
    res = GetOverlappedResult(_file, &pending_write.overlapped, &bytes_written, FALSE);
    Sleep(1);
  }
}

/*
TODO:
unsave Skyline packer data, directly use a new texture when preloading
file operate should in file manageer
shuold i use corountine with IOCP impl async read write?
impl font pregenerate cache files
*/
void GlyphCacher::save() noexcept
{
  assert(_file != INVALID_HANDLE_VALUE);

  // Serialize glyphs data
  pending_write.data = serialize();
  _glyphs.clear();

  pending_write.overlapped.Offset = static_cast<DWORD>(_write_offset);
  pending_write.overlapped.OffsetHigh = static_cast<DWORD>(_write_offset >> 32);
  auto bytes = static_cast<DWORD>(pending_write.data.size());

  auto res = WriteFile(_file, pending_write.data.data(), bytes, nullptr, &pending_write.overlapped);
  err_if(!res && GetLastError() != ERROR_IO_PENDING, "Failed to WriteFile with overlapped");

  _write_offset += bytes;

  _cache_size = 0; 
}

namespace {

static constexpr auto Magic = static_cast<uint>('T') | static_cast<uint>('K') << 8 | static_cast<uint>('G') << 16 | static_cast<uint>('C') << 24;

}

auto GlyphCacher::serialize() const noexcept -> std::vector<uint8>
{
  auto data = std::vector<uint8>(std::ranges::fold_left(_glyphs, 0u, [](auto size, auto const& glyph)
    { return size + sizeof(glyph.header) + glyph.bitmap.size(); }) + (_write_offset ? 0 : sizeof(Magic)));

  auto ptr = data.data();
  if (_write_offset == 0)
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
  assert(_file == INVALID_HANDLE_VALUE);
  std::filesystem::create_directories(".cache"); // TODO: impl auto create directory in g_file_manager
  _file = CreateFileA(".cache/glyphs.bin", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
  assert(_file != INVALID_HANDLE_VALUE);

  LARGE_INTEGER size{};
  err_if(!GetFileSizeEx(_file, &size), "Failed to get file size");
  _write_offset = size.QuadPart;

  if (!_write_offset) return;

  auto data = std::vector<uint8>(_write_offset);
  OVERLAPPED read{};
  DWORD bytes_read{};
  auto res = ReadFile(_file, data.data(), static_cast<DWORD>(data.size()), &bytes_read, &read);
  err_if(!res && GetLastError() != ERROR_IO_PENDING, "Failed to read file");
  res = GetOverlappedResult(_file, &read, &bytes_read, TRUE);
  err_if(!res, "Failed to get overlapped result in read file");

  deserialize(data);
}

void GlyphCacher::deserialize(std::span<uint8 const> data) noexcept
{
  assert(data.size() > sizeof(Magic));
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
