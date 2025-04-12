//
// descriptor allocator
//

#pragma once

namespace tk { namespace graphics_engine {

  class DescriptorAllocator
  {
  public:
    DescriptorAllocator();
    ~DescriptorAllocator();

    DescriptorAllocator(DescriptorAllocator const&)            = delete;
    DescriptorAllocator(DescriptorAllocator&&)                 = delete;
    DescriptorAllocator& operator=(DescriptorAllocator const&) = delete;
    DescriptorAllocator& operator=(DescriptorAllocator&&)      = delete;

  private:

  };

} }
