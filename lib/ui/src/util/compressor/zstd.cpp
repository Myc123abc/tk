#include "compressor.hpp"
#include "tk/error_handling.hpp"

#include <zstd.h>

namespace tk {

auto Compressor::compression_zstd(std::span<uint8 const> data, int level) const noexcept -> std::vector<uint8>
{
  auto res = std::vector<uint8>(ZSTD_compressBound(data.size()));
  auto const compressed_size = ZSTD_compress(res.data(), res.size(), data.data(), data.size(),
    std::clamp(level, zstd::min_level(), zstd::max_level()));
  err_if(static_cast<bool>(ZSTD_isError(compressed_size)),
    "Zstd compression failed: {}", ZSTD_getErrorName(compressed_size));
  res.resize(compressed_size);
  return res;
}

auto Compressor::decompression_zstd(std::span<uint8 const> data) const noexcept -> std::vector<uint8>
{
  auto remaining             = data;
  auto decompressed_capacity = uint64{};
  while (!remaining.empty())
  {
    auto frame_size = ZSTD_findFrameCompressedSize(remaining.data(), remaining.size());
    err_if(static_cast<bool>(ZSTD_isError(frame_size)),
      "failed to find zstd frame size: {}", ZSTD_getErrorName(frame_size));

    auto frame_content_size = ZSTD_getFrameContentSize(remaining.data(), frame_size);
    err_if(frame_content_size == ZSTD_CONTENTSIZE_UNKNOWN || frame_content_size == ZSTD_CONTENTSIZE_ERROR,
      "failed to get zstd frame content size");

    decompressed_capacity += frame_content_size;
    remaining = remaining.subspan(frame_size);
  }

  auto res               = std::vector<uint8>(decompressed_capacity);
  auto decompressed_size = ZSTD_decompress(res.data(), res.size(), data.data(), data.size());
  err_if(static_cast<bool>(ZSTD_isError(decompressed_size)),
    "zstd decompression failed: {}", ZSTD_getErrorName(decompressed_size));
  err_if(decompressed_size != res.size(),
    "zstd decompression size mismatch: expected {}, got {}",
    res.size(), decompressed_size);
  return res;
}

auto zstd::max_level() noexcept -> int
{
  return ZSTD_maxCLevel();
}

auto zstd::min_level() noexcept -> int
{
  return ZSTD_minCLevel();
}

auto zstd::default_level() noexcept -> int
{
  return ZSTD_defaultCLevel();
}

}
