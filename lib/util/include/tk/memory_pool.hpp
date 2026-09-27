#pragma once

#include "free_list.hpp"
#include "tuple.hpp"

#include <cassert>
#include <vector>
#include <bit>

namespace tk {

constexpr auto operator""_B(size_t x) noexcept -> size_t
{
  return x;
}

constexpr auto operator""_KiB(size_t x) noexcept -> size_t
{
  return 1024ULL * x;
}

constexpr auto operator""_MiB(size_t x) noexcept -> size_t
{
  return 1024_KiB * x;
}

constexpr auto operator""_GiB(size_t x) noexcept -> size_t
{
  return 1024_MiB * x;
}

constexpr auto operator""_TiB(size_t x) noexcept -> size_t
{
  return 1024_TiB * x;
}

constexpr auto operator""_PiB(size_t x) noexcept -> size_t
{
  return 1024_PiB * x;
}

template <size_t... Sizes>
class MemoryPool
{
  static constexpr auto ChunkCapacity = 64_KiB;
  static constexpr auto MaxSize       = std::max({Sizes...});

  static consteval auto valid_sizes() noexcept
  {
    constexpr size_t sizes[] = { Sizes... };
    for (auto i = 1; i < sizeof...(Sizes); ++i)
    {
      if (sizes[i - 1] >= sizes[i])
        return false;
    }
    return true;
  }

  static_assert(valid_sizes(), "Pool sizes must be strictly ascending and unique");

public:
  MemoryPool()                             = default;
  ~MemoryPool()                            = default;
  MemoryPool(MemoryPool const&)            = delete;
  MemoryPool(MemoryPool&&)                 = delete;
  MemoryPool& operator=(MemoryPool const&) = delete;
  MemoryPool& operator=(MemoryPool&&)      = delete;

private:
  struct ChunkHeader
  {
    void* pool;
    void (*free)(void* pool, void* p) noexcept;
  };

  static_assert(std::has_single_bit(ChunkCapacity) && ChunkCapacity >= sizeof(ChunkHeader));

public:
  auto alloc(size_t size) noexcept -> void*
  {
    assert(size <= MaxSize);
    void* result{};
    _pools.apply([&](auto&... pool) { ((result = pool.alloc(size)) || ...); });
    return result;
  }

  void free(void*& p) noexcept
  {
    assert(p);
    auto address = reinterpret_cast<std::uintptr_t>(p);
    auto chunk = reinterpret_cast<ChunkHeader*>(address & ~(ChunkCapacity - 1));
    chunk->free(chunk->pool, p);
    p = nullptr;
  }

  void destroy() noexcept
  {
    _pools.apply([&](auto&... pool) { (pool.destroy(), ...); });
  }

private:
  template <size_t Size>
  struct Pool
  {
    static constexpr auto Aligment       = alignof(std::max_align_t);
    static constexpr auto DataOffset     = (sizeof(ChunkHeader) + Aligment - 1) / Aligment * Aligment;
    static constexpr auto DataCapacity   = ChunkCapacity - DataOffset;
    static constexpr auto ChunkCount     = DataCapacity / Size;
    static constexpr auto UsableCapacity = ChunkCount * Size;
    static constexpr auto MaxGrowChunks  = 64;

    static_assert(Size >= sizeof(FreeList), "Pool size is too small for the free-list node");
    static_assert(Size % Aligment == 0, "Pool size must preserve alignment");
    static_assert(Size <= DataCapacity, "Pool size is larger than the chunk");

    FreeList           _free_list;
    std::vector<void*> _chunks;
    size_t             _curr_chunk{};
    size_t             _curr_offset{};
    size_t             _grow_chunks = 1;

    void destroy() noexcept
    {
      for (auto p : _chunks) _aligned_free(p);
      _chunks.clear();
      _free_list.clear();
      _curr_chunk  = 0;
      _curr_offset = 0;
      _grow_chunks = 1;
    }

    auto alloc(size_t size) noexcept -> void*
    {
      if (size <= Size) return alloc();
      return nullptr;
    }

    auto alloc() noexcept -> void*
    {
      auto result = _free_list.pop();
      return result ? result : alloc_new();
    }

    auto alloc_new() noexcept -> void*
    {
      assert(_curr_offset <= UsableCapacity);
      if (_chunks.empty())
      {
        grow_chunks();
      }
      else if (_curr_offset == UsableCapacity)
      {
        ++_curr_chunk;
        _curr_offset = 0;
        if (_curr_chunk == _chunks.size()) grow_chunks();
      }

      auto ptr = static_cast<std::byte*>(_chunks[_curr_chunk]) + DataOffset + _curr_offset;
      _curr_offset += Size;
      return ptr;
    }

    void grow_chunks() noexcept
    {
      auto chunk_count = _grow_chunks;
      _chunks.reserve(_chunks.size() + chunk_count);
      while (chunk_count-- > 0) add_chunk();
      if (_grow_chunks < MaxGrowChunks) _grow_chunks = std::min(_grow_chunks * 2, MaxGrowChunks);
    }

    void add_chunk() noexcept
    {
      auto chunk = _chunks.emplace_back(_aligned_malloc(ChunkCapacity, ChunkCapacity));
      assert(chunk);
      auto header = static_cast<ChunkHeader*>(chunk);
      header->pool = this;
      header->free = &free_from_chunk;
    }

    static void free_from_chunk(void* pool, void* p) noexcept
    {
      reinterpret_cast<Pool*>(pool)->free(p);
    }

    void free(void* p) noexcept
    {
      _free_list.push(p);
    }
  };

  Tuple<Pool<Sizes>...> _pools;
};

using DefaultMemoryPool = MemoryPool<
  16_B,
  32_B,
  64_B,
  128_B,
  256_B,
  512_B,
  1_KiB,
  2_KiB,
  4_KiB>;

template <>
class MemoryPool<> : public DefaultMemoryPool {};

MemoryPool() -> MemoryPool<>;

}
