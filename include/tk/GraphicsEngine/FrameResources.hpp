//
// frame resources
//

#pragma once

#include "CommandPool.hpp"
#include "Swapchain.hpp"
#include "MemoryAllocator.hpp"
#include "../ErrorHandling.hpp"

#include <queue>
#include <functional>


namespace tk { namespace graphics_engine {

struct FrameResource
{
  Command     cmd;
  VkFence     fence;
  VkSemaphore acquire_sem;
};

class FrameResources;

class FramesDynamicBuffer
{
  enum class State
  {
    append_data,
    uploaded,
  }; 

public:
  FramesDynamicBuffer()            = default;
  FramesDynamicBuffer(auto const&) = delete;
  FramesDynamicBuffer(auto&&)      = delete;
  auto operator=(auto const&)      = delete;
  auto operator=(auto&&)           = delete;

  void init(FrameResources* frame_resources, MemoryAllocator* alloc);
  void destroy();

  void frame_begin();

  auto get_current_frame_byte_offset() const -> uint32_t;

  template <typename T>
  requires std::ranges::sized_range<T>      &&
           std::ranges::contiguous_range<T>
  void append_range(T&& values)
  {
    throw_if(_state != State::append_data, "[VertexBuffer] cannot append data after upload");
    // no value return
    auto count = std::ranges::size(values);
    if (count == 0) return;

    // get value type
    using ValueType = std::ranges::range_value_t<T>;

    // get values byte size and current frame data size
    auto values_byte_size = count * sizeof(ValueType);
    auto byte_offset      = _current_frame_data.size();

    // resize frame data
    _current_frame_data.resize(byte_offset + values_byte_size);

    memcpy(_current_frame_data.data() + byte_offset, std::ranges::data(values), values_byte_size);
  }

  void upload();
  
  auto get_handle_and_address() const -> std::pair<VkBuffer, VkDeviceAddress>;

  auto size() const -> uint32_t;

private:
  FrameResources*        _frame_resources{};
  Buffer                 _buffer;
  uint32_t               _byte_size_pre_frame;
  uint32_t               _current_frame_byte_offset{};
  std::vector<std::byte> _current_frame_data;
  State                  _state = State::append_data;
  MemoryAllocator*       _alloc{};
};

class FrameResources
{
public:
  FrameResources()            = default;
  FrameResources(auto const&) = delete;
  FrameResources(auto&&)      = delete;
  auto operator=(auto const&) = delete;
  auto operator=(auto&&)      = delete;

  void init(VkDevice device, CommandPool& cmd_pool, Swapchain* swapchain);
  void destroy();

  auto& get_command() const noexcept { return _frames[_frame_index].cmd; }

  auto acquire_swapchain_image(bool wait) -> bool;
  void present_swapchain_image(VkQueue graphics_queue, VkQueue present_queue);
  auto& get_swapchain_image() noexcept { return _swapchain->image(_submit_sem_index); }

  void destroy_old_resources();
  auto get_current_frame_index() const noexcept { return _frame_index; }
  auto size() const noexcept { return _frames.size(); }
  auto push_old_resource(std::function<void()>&& func)
  {
    _destructors.push(
      [frame_index = _frame_index, func]
      (uint32_t current_frame_index, bool directly_destruct)
      {
        if (directly_destruct || current_frame_index == frame_index)
        {
          func();
          return true;
        }
        return false;
      });
  }

private:
  VkDevice                   _device;
  std::vector<FrameResource> _frames;
  std::vector<VkSemaphore>   _submit_sems;
  uint32_t                   _frame_index{};
  uint32_t                   _submit_sem_index{};
  Swapchain*                 _swapchain{};

  std::queue<std::function<bool(uint32_t, bool)>> _destructors;
};

}}