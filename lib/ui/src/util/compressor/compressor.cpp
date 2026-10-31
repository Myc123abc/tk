#include "compressor.hpp"
#include "tk/error_handling.hpp"

namespace tk {

auto Compressor::compression(std::span<uint8 const> data, CompressionAlgorithm algorithm) const noexcept -> std::vector<uint8>
{
  err_if(data.empty(), "compression input is null or empty");
  return algorithm.visit
  (
    [&](zstd cfg) { return compression_zstd(data, cfg.level); }
  );
}

auto Compressor::decompression(std::span<uint8 const> data, CompressionAlgorithm algorithm) const noexcept -> std::vector<uint8>
{
  err_if(data.empty(), "decompression input is null or empty");
  return algorithm.visit
  (
    [&](zstd) { return decompression_zstd(data); }
  );
}

}
