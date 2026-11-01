#pragma once

#include "tk/base.hpp"
#include "tk/string_id.hpp"
#include "tk/error_handling.hpp"

#include <span>
#include <vector>

namespace tk {

class BinaryFileFormat
{
public:
  BinaryFileFormat()                                   = default;
  ~BinaryFileFormat()                                  = default;
  BinaryFileFormat(BinaryFileFormat const&)            = delete;
  BinaryFileFormat(BinaryFileFormat&&)                 = delete;
  BinaryFileFormat& operator=(BinaryFileFormat const&) = delete;
  BinaryFileFormat& operator=(BinaryFileFormat&&)      = delete;

  void set_magic(StringID magic) noexcept { _magic = magic; }

  void read(std::span<uint8 const> data) noexcept { _read_data = data; }
  auto is_read_data_empty() const noexcept { return _read_data.empty(); }

  auto read_magic() noexcept -> uint64
  {
    err_if(!_magic, "failed to read magic, magic is not set.");
    err_if(_read_data.size() < sizeof(_magic), "failed to read magic, file size is too small.");
    auto magic = uint64{};
    memcpy(&magic, _read_data.data(), sizeof(_magic));
    err_if(magic != _magic, "failed to read magic, magic is not same.");
    _read_data = _read_data.subspan(sizeof(_magic));
    return magic;
  }

  template <typename T>
  auto read_field() noexcept -> std::pair<T, std::span<uint8 const>>
  {
    static_assert(std::is_trivially_copyable_v<T>);

    auto header_size = sizeof(T);
    auto size_size   = sizeof(uint64);

    assert(_read_data.size() >= header_size + size_size);

    T      header;
    uint64 size;

    memcpy(&header, _read_data.data(), header_size);
    memcpy(&size, _read_data.data() + header_size, size_size);

    auto offset = header_size + size_size;
    assert(_read_data.size() >= offset + size);

    auto data = _read_data.subspan(offset, size);
    _read_data = _read_data.subspan(offset + size);

    return { header, data };
  }

  template <typename T>
  void append(T&& header, std::vector<uint8> const& data) noexcept
  {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>);
    _fields.emplace_back
    (
      std::span<uint8 const>{ reinterpret_cast<uint8 const*>(&header), sizeof(header) },
      std::span<uint8 const>{ data.data(), data.size() }
    );
  }

  void build() noexcept
  {
    // get total byte size
    auto size = uint64{};
    if (_magic) size += sizeof(_magic);
    for (auto [header, data] : _fields)
    {
      size += header.size();
      size += sizeof(uint64);
      size += data.size();
    }
  
    // copy data
    _buf.resize(size);
    auto p = _buf.data();
    if (_magic)
    {
      memcpy(p, &_magic, sizeof(_magic));
      p += sizeof(_magic);
    }
    for (auto [header, data] : _fields)
    {
      memcpy(p, header.data(), header.size());
      p += header.size();
      auto size = static_cast<uint64>(data.size());
      memcpy(p, &size, sizeof(size));
      p += sizeof(size);
      memcpy(p, data.data(), data.size());
      p += data.size();
    }
  }

  auto data() const noexcept -> std::span<uint8 const>
  {
    return { _buf.data(), _buf.size() };
  }

private:
  struct Field
  {
    std::span<uint8 const> header;
    std::span<uint8 const> data;
  };
  uint64                 _magic{};
  std::vector<Field>     _fields;
  std::vector<uint8>     _buf;
  std::span<uint8 const> _read_data;
};

}
