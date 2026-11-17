module;

#include <vector>
#include <span>

export module tk.compressor;

import tk.base;
import tk.variant;
import tk.singleton;

export namespace tk {

struct zstd
{
  static auto min_level() noexcept -> int;
  static auto max_level() noexcept -> int;
  static auto default_level() noexcept -> int;

  int level{ default_level() };
};

using CompressionAlgorithm = Variant<zstd>;

class Compressor : public Singleton<Compressor>
{
public:
  auto compression(std::span<uint8 const> data, CompressionAlgorithm algorithm = zstd{}) const noexcept -> std::vector<uint8>;
  auto decompression(std::span<uint8 const> data, CompressionAlgorithm algorithm = zstd{}) const noexcept -> std::vector<uint8>;

private:
  auto compression_zstd(std::span<uint8 const> data, int level) const noexcept -> std::vector<uint8>;
  auto decompression_zstd(std::span<uint8 const> data) const noexcept -> std::vector<uint8>;
};

auto& g_compressor = Compressor::instance();

}
