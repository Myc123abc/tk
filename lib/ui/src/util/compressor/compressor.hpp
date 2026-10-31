#pragma once

#include "../singleton.hpp"
#include "tk/base.hpp"
#include "tk/variant.hpp"

#include <vector>
#include <span>

namespace tk {

struct zstd
{
  static auto min_level() noexcept -> int;
  static auto max_level() noexcept -> int;
  static auto default_level() noexcept -> int;

  int level{ default_level() };
};

using CompressionAlgorithm = Variant<zstd>;

Singleton(Compressor, g_compressor,
public:
  auto compression(std::span<uint8 const> data, CompressionAlgorithm algorithm = zstd{}) const noexcept -> std::vector<uint8>;
  auto decompression(std::span<uint8 const> data, CompressionAlgorithm algorithm = zstd{}) const noexcept -> std::vector<uint8>;

private:
  auto compression_zstd(std::span<uint8 const> data, int level) const noexcept -> std::vector<uint8>;
  auto decompression_zstd(std::span<uint8 const> data) const noexcept -> std::vector<uint8>;
)

}
