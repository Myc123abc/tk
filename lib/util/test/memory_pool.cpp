#include "tk/memory_pool.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

using namespace tk;

auto main() -> int
{
  {
    MemoryPool<16_B, 32_B> pool;

    auto p = pool.alloc(8);
    assert(p);
    assert(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t) == 0);

    auto p_address = p;
    pool.free(p);
    assert(!p);
    assert(pool.alloc(8) == p_address);

    pool.destroy();
  }

  {
    MemoryPool<> default_pool;
    auto p = default_pool.alloc(64);
    assert(p);
    default_pool.free(p);
    assert(!p);

    MemoryPool deduced_pool;
    auto q = deduced_pool.alloc(128);
    assert(q);

    default_pool.destroy();
    deduced_pool.destroy();
  }

  {
    MemoryPool<16_B> pool;
    std::vector<void*> pointers;
    pointers.reserve(5000);

    for (auto i = 0; i < 5000; ++i)
    {
      auto p = pool.alloc(16);
      assert(p);
      pointers.push_back(p);
    }

    for (auto& p : pointers)
    {
      pool.free(p);
      assert(!p);
    }

    pool.destroy();

    auto p = pool.alloc(16);
    assert(p);
    pool.free(p);
    assert(!p);
    pool.destroy();
  }

  {
    MemoryPool<16_B, 32_B, 64_B, 128_B> pool;
    std::array<size_t, 7> sizes{ 1, 16, 17, 32, 33, 64, 128 };

    for (auto size : sizes)
    {
      auto p = pool.alloc(size);
      assert(p);
      assert(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t) == 0);
      pool.free(p);
      assert(!p);
    }

    pool.destroy();
  }
}
