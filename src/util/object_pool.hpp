#pragma once

#include "util/error_handling.hpp"
#include "util/base.hpp"

#include <array>
#include <vector>
#include <memory>
#include <assert.h>

namespace tk {

template <typename T, uint16 BlockCapacity>
requires (BlockCapacity > 0)                                   &&
         (BlockCapacity <= std::numeric_limits<uint16>::max()) &&
         std::is_nothrow_constructible_v<T>                    &&
         std::is_nothrow_destructible_v<T>
class ObjectPool;

template <typename T, uint16 BlockCapacity>
class [[nodiscard]] ObjectPoolHandle
{
  friend class ObjectPool<T, BlockCapacity>;
  friend struct std::hash<ObjectPoolHandle>;

public:
  constexpr ObjectPoolHandle() noexcept = default;

  constexpr auto valid() const noexcept { return _generation != 0; }

  constexpr explicit operator bool() const noexcept { return valid(); }

  constexpr bool operator==(ObjectPoolHandle const&) const noexcept = default;

private:
  constexpr ObjectPoolHandle(uint16 block_idx, uint16 slot_idx, uint generation) noexcept
    : _block_idx(block_idx), _slot_idx(slot_idx), _generation(generation) {}

  constexpr auto pack() const noexcept
  {
    return static_cast<uint64>(_generation) << 32 |
           static_cast<uint64>(_slot_idx)   << 16 |
           static_cast<uint64>(_block_idx);
  }

private:
  uint16 _block_idx{};
  uint16 _slot_idx{};
  uint   _generation{};
};

template <typename T, uint16 BlockCapacity = 32>
requires (BlockCapacity > 0)                                   &&
         (BlockCapacity <= std::numeric_limits<uint16>::max()) &&
         std::is_nothrow_constructible_v<T>                    &&
         std::is_nothrow_destructible_v<T>
class ObjectPool
{
public:
  using Handle = ObjectPoolHandle<T, BlockCapacity>;

  ObjectPool() noexcept
  {
    allocate_block();
  }

  ~ObjectPool() noexcept
  {
    err_if(_alive_count != 0, "[ObjectPool] Failed to destruct ObjectPool. Still have objects are undestroied");
  }

  ObjectPool(ObjectPool const&)            = delete;
  ObjectPool(ObjectPool&&)                 = delete;
  ObjectPool& operator=(ObjectPool const&) = delete;
  ObjectPool& operator=(ObjectPool&&)      = delete;

  [[nodiscard]]
  auto alloc() noexcept -> Handle
  {
    if (_free_head != InvalidFreeSlot)
    {
      auto [block_idx, slot_idx] = unpack_slot_index(_free_head);
      auto slot = get_slot(block_idx, slot_idx);
      assert(!slot->alive && slot->generation);
      _free_head = slot->next_free;
      std::construct_at(slot->get());
      slot->alive     = true;
      slot->next_free = InvalidFreeSlot;
      ++_alive_count;
      return Handle{ static_cast<uint16>(block_idx), static_cast<uint16>(slot_idx), slot->generation };
    }

    if (_block_idx == _blocks.size())
      allocate_block();

    auto block_idx = _block_idx;
    auto slot_idx  = _slot_idx;
    auto slot      = get_slot(block_idx, slot_idx);
    assert(!slot->alive && !slot->generation);
    std::construct_at(slot->get());
    slot->alive      = true;
    slot->generation = 1;
    slot->next_free  = InvalidFreeSlot;
    ++_alive_count;

    advance_alloc_cursor();
    return Handle{ static_cast<uint16>(block_idx), static_cast<uint16>(slot_idx), slot->generation };
  }

  [[nodiscard]]
  auto get(Handle handle) noexcept -> T*
  {
    assert(handle.valid());
    auto slot = get_slot(handle._block_idx, handle._slot_idx);
    assert(slot->alive && slot->generation == handle._generation);
    return slot->get();
  }

  auto operator[](Handle handle) noexcept -> T&
  {
    return *get(handle);
  }

  [[nodiscard]]
  auto get(Handle handle) const noexcept -> T const*
  {
    assert(handle.valid());
    auto slot = get_slot(handle._block_idx, handle._slot_idx);
    assert(slot->alive && slot->generation == handle._generation);
    return slot->get();
  }

  auto operator[](Handle handle) const noexcept -> T const&
  {
    return *get(handle);
  }

  void free(Handle& handle) noexcept
  {
    assert(handle.valid());
    auto slot = get_slot(handle._block_idx, handle._slot_idx);
    assert(slot->alive && slot->generation == handle._generation);
    std::destroy_at(slot->get());
    slot->alive = false;
    ++slot->generation;
    err_if(slot->generation == std::numeric_limits<decltype(slot->generation)>::max(),
      "[ObjectPool] Failed to destroy object, exceed the max slot generation");
    slot->next_free = _free_head;
    _free_head = pack_slot_index(handle._block_idx, handle._slot_idx);
    --_alive_count;
    handle = {};
  }

private:
  static constexpr auto InvalidFreeSlot = std::numeric_limits<uint>::max();

  static constexpr auto pack_slot_index(uint16 block_idx, uint16 slot_idx) noexcept -> uint
  {
    return static_cast<uint>(block_idx) << 16 | slot_idx;
  }

  static constexpr auto unpack_slot_index(uint slot_index) noexcept
  {
    return std::pair{
      static_cast<uint16>(slot_index >> 16),
      static_cast<uint16>(slot_index & std::numeric_limits<uint16>::max())
    };
  }

  void allocate_block() noexcept
  {
    err_if(_blocks.size() > std::numeric_limits<uint16>::max(),
      "[ObjectPool] Failed to allocate new block, exceed the max block capacity");
    _blocks.emplace_back(std::make_unique<Block>());
  }

  void advance_alloc_cursor() noexcept
  {
    if (++_slot_idx == BlockCapacity)
    {
      _slot_idx = 0;
      ++_block_idx;
    }
  }

  auto get_slot(size_t block_idx, uint16 slot_idx) const noexcept
  {
    assert(block_idx < _blocks.size() && slot_idx < BlockCapacity);
    return &(*_blocks[block_idx])[slot_idx];
  }

private:
  struct Slot
  {
    alignas(T) std::byte obj[sizeof(T)];
    uint                 generation{};
    uint                 next_free{ InvalidFreeSlot };
    bool                 alive{};

    auto get() noexcept -> T*
    {
      return std::launder(reinterpret_cast<T*>(obj));
    }

    auto get() const noexcept -> T const*
    {
      return std::launder(reinterpret_cast<T const*>(obj));
    }
  };

  using Block = std::array<Slot, BlockCapacity>;

  std::vector<std::unique_ptr<Block>> _blocks;
  uint                                _free_head{ InvalidFreeSlot };
  size_t                              _alive_count{};
  size_t                              _block_idx{};
  uint16                              _slot_idx{};
};

}

namespace std {

template <typename T, tk::uint16 BlockCapacity>
struct hash<tk::ObjectPoolHandle<T, BlockCapacity>>
{
  auto operator()(tk::ObjectPoolHandle<T, BlockCapacity> const& h) const noexcept -> size_t
  {
    return std::hash<tk::uint64>{}(h.pack());
  }
};

}
