#include "tk/GraphicsEngine/MemoryAllocator.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/GraphicsEngine/CommandPool.hpp"
#include "tk/GraphicsEngine/MaterialLibrary.hpp"

namespace tk { namespace graphics_engine {

void MemoryAllocator::init(VkPhysicalDevice physical_device, VkDevice device, VkInstance instance, uint32_t vulkan_version)
{
  _device = device;
  VmaAllocatorCreateInfo alloc_info
  {
    .flags            = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice   = physical_device,
    .device           = device,
    .instance         = instance,
    .vulkanApiVersion = vulkan_version,
  };
  throw_if(vmaCreateAllocator(&alloc_info, &_allocator) != VK_SUCCESS,
           "failed to create Vulkan Memory Allocator");
}

void MemoryAllocator::destroy()
{
  if (_allocator != VK_NULL_HANDLE)
    vmaDestroyAllocator(_allocator);
  _device    = VK_NULL_HANDLE;
  _allocator = VK_NULL_HANDLE;
}

auto MemoryAllocator::create_buffer(uint32_t size, VkBufferUsageFlags usages, VmaAllocationCreateFlags flags) -> Buffer
{
  Buffer buffer;
  VkBufferCreateInfo buf_info
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size  = size,
    .usage = usages,
  };
  VmaAllocationCreateInfo alloc_info
  {
    .flags = flags,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };
  throw_if(vmaCreateBuffer(_allocator, &buf_info, &alloc_info, &buffer.handle, &buffer.allocation, nullptr) != VK_SUCCESS,
           "failed to create buffer");
  return buffer;
}

void MemoryAllocator::destroy_buffer(Buffer& buffer)
{
  if (buffer.handle != VK_NULL_HANDLE && buffer.allocation != VK_NULL_HANDLE)
    vmaDestroyBuffer(_allocator, buffer.handle, buffer.allocation);
  buffer = {};
}

auto MemoryAllocator::create_mesh_buffer(Command& command, std::vector<Mesh>& meshs, DestructorStack& destructor, std::vector<MeshInfo>& mesh_infos) -> MeshBuffer
{
  MeshBuffer buffer;

  // get mesh infos
  mesh_infos.reserve(meshs.size());
  uint32_t vertices_offset = 0, indices_offset = 0;
  auto vertices      = std::vector<Vertex>();
  auto indices       = std::vector<uint16_t>();
  for (auto const& mesh : meshs)
  {
    uint32_t vertices_byte_size = sizeof(Vertex)  * mesh.vertices.size();
    uint32_t indices_byte_size  = sizeof(uint8_t) * mesh.indices.size();
    mesh_infos.emplace_back(MeshInfo
    {
      .vertices_offset = vertices_offset,
      .indices_offset  = indices_offset,
      .indices_count   = (uint32_t)mesh.indices.size(),
    });
    vertices_offset += vertices_byte_size;
    indices_offset  += indices_byte_size;
    vertices.append_range(mesh.vertices);
    indices.append_range(mesh.indices);
  }

  // create mesh buffer 
  uint32_t vertices_byte_size = sizeof(Vertex)   * vertices.size();
  int32_t  indices_byte_size  = sizeof(uint16_t) * indices.size();
  buffer.vertices = create_buffer(vertices_byte_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT        |
                                                      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  buffer.indices  = create_buffer(indices_byte_size,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  // get address
  VkBufferDeviceAddressInfo info
  {
    .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = buffer.vertices.handle,
  };
  buffer.address = vkGetBufferDeviceAddress(_device, &info);
    
  // create stage buffer
  auto stage = create_buffer(vertices_byte_size + indices_byte_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  // copy mesh data to stage buffer
  throw_if(vmaCopyMemoryToAllocation(_allocator, vertices.data(), stage.allocation, 0, vertices_byte_size) != VK_SUCCESS,
           "failed to copy vertices data to stage buffer");
  throw_if(vmaCopyMemoryToAllocation(_allocator, indices.data(), stage.allocation, vertices_byte_size, indices_byte_size) != VK_SUCCESS,
           "failed to copy indices data to stage buffer");

  // transform data from stage to mesh buffer
  VkBufferCopy copy
  {
    .size = vertices_byte_size,
  };
  vkCmdCopyBuffer(command, stage.handle, buffer.vertices.handle, 1, &copy);
  copy.size      = indices_byte_size;
  copy.srcOffset = vertices_byte_size;
  vkCmdCopyBuffer(command, stage.handle, buffer.indices.handle, 1, &copy);

  destructor.push([stage, this] mutable { destroy_buffer(stage); });

  return buffer;
}

void MemoryAllocator::destroy_mesh_buffer(MeshBuffer& buffer)
{
  destroy_buffer(buffer.vertices);
  destroy_buffer(buffer.indices);
  buffer = {};
}

}}
