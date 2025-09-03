//
// frame resources
//

#pragma once

#include "CommandPool.hpp"
#include "Swapchain.hpp"
#include "MemoryAllocator.hpp"
#include "../ErrorHandling.hpp"
#include "config.hpp"
#include "../util.hpp"

#include <queue>
#include <functional>


namespace tk { namespace graphics_engine {

struct FrameResource
{
  Command     cmd;
  VkFence     fence;
  VkSemaphore acquire_sem;
};

#if 0
class VertexBuffer
{
  enum class State
  {
    append_data,
    uploaded,
  };

  friend class FrameResources;
private:
  void init(FrameResources* frame_resources, MemoryAllocator& alloc);
  void destroy() const;

  void frame_begin() noexcept;

public:
  template <typename ContainerType>
  void append_range(ContainerType const& c)
  {
    throw_if(_state != State::append_data, "[VertexBuffer] append data after upload!");
    using ValueType = typename ContainerType::value_type;
    static_assert(std::is_trivially_copyable_v<ValueType>, "must be copyable type");
    auto size = c.size() * sizeof(ValueType);
    _current_frame_data.resize(_current_frame_data.size() + size);
    memcpy(_current_frame_data.data() + _current_frame_data.size(), c.data(), size);
  }

  void upload();

  auto get_handle_and_address() const -> std::pair<VkBuffer, VkDeviceAddress>;

private:
  FrameResources*        _frame_resources;
  Buffer                 _buffer;
  std::vector<std::byte> _current_frame_data;
  State                  _state = State::append_data;
  uint32_t               _size_pre_frame{};
};
#endif

class FrameResources
{
  //friend class VertexBuffer;
public:
  FrameResources() = default;

  void init(VkDevice device, CommandPool& cmd_pool, Swapchain* swapchain, MemoryAllocator& alloc);
  void destroy();

  auto& get_command() const noexcept { return _frames[_frame_index].cmd; }

  auto acquire_swapchain_image(bool wait) -> bool;
  void present_swapchain_image(VkQueue graphics_queue, VkQueue present_queue);
  auto& get_swapchain_image() noexcept { return _swapchain->image(_submit_sem_index); }

private:
  VkDevice                   _device;
  std::vector<FrameResource> _frames;
  std::vector<VkSemaphore>   _submit_sems;
  uint32_t                   _frame_index{};
  uint32_t                   _submit_sem_index{};
  Swapchain*                 _swapchain{};
  //VertexBuffer               _vertex_buffer;

  enum class State
  {
    append_data,
    uploaded,
  };

  Buffer                 _vertex_buffer;
  uint32_t               _byte_size_pre_frame;
  uint32_t               _current_frame_byte_offset{};
  std::vector<std::byte> _current_frame_data;
  State                  _state = State::append_data;
  MemoryAllocator*       _alloc{};
  std::queue<std::function<bool(uint32_t, bool)>> _destructors;
  
public:

  auto get_current_frame_byte_offset() const
  {
    throw_if(_state != State::uploaded, "[VertexBuffer] cannot get frame byte offset after upload");
    return _current_frame_byte_offset;
  }

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

  void upload()
  {
    // exceed capacity, need to expand
    if (_current_frame_data.size() > _byte_size_pre_frame)
    {
      _byte_size_pre_frame = util::align_size(std::max(_current_frame_data.size(), static_cast<size_t>(_byte_size_pre_frame * config()->buffer_expand_ratio)), 8);
      auto tmp_buf = _alloc->create_buffer(_byte_size_pre_frame * _frames.size(), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
      _destructors.emplace(
        [buf = this->_vertex_buffer, frame_index = this->_frame_index]
        (uint32_t current_frame_index, bool directly_destruct)
        {
          if (directly_destruct)
          {
            buf.destroy();
            return true;
          }
          if (current_frame_index == frame_index)
          {
            buf.destroy();
            return true;
          }
          return false;
        });
      _vertex_buffer.set_realloc_info(tmp_buf);
      _current_frame_byte_offset = _frame_index * _byte_size_pre_frame;
      upload();
      return;
    }

    memcpy(static_cast<std::byte*>(_vertex_buffer.data()) + _current_frame_byte_offset, _current_frame_data.data(), _current_frame_data.size());
    _current_frame_data.clear();
    _state = State::uploaded;
  }
  
  // set state is promise the handle and address is valid
  // append_range maybe recreate a bigger buffer to make handle and address invalid
  auto get_handle_and_address() const -> std::pair<VkBuffer, VkDeviceAddress>
  {
    throw_if(_state != State::uploaded, "[VertexBuffer] must be uploaded, then handle and address will be valid");
    return { _vertex_buffer.handle(), _vertex_buffer.address() + _current_frame_byte_offset };
  }

  auto size() const
  {
    throw_if(_state != State::append_data, "[VertexBuffer] cannot get current frame data size after upload");
    return _current_frame_data.size();
  }
};

}}