#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/ErrorHandling.hpp"
#include "tk/log.hpp"

namespace tk { namespace graphics_engine {

////////////////////////////////////////////////////////////////////////////////
//                               Image 
////////////////////////////////////////////////////////////////////////////////

void GraphicsEngine::transition_image_layout(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
  auto aspect_mask = new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL ?
                     VK_IMAGE_ASPECT_DEPTH_BIT :
                     VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageMemoryBarrier2 barrier
  {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    // HACK: use all commands bit will stall the GPU pipeline a bit, is inefficient.
    // should make stageMask more accurate.
    // reference: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
    .srcStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .srcAccessMask    = VK_ACCESS_2_MEMORY_WRITE_BIT,
    .dstStageMask     = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    .dstAccessMask    = VK_ACCESS_2_MEMORY_READ_BIT  |
                        VK_ACCESS_2_MEMORY_WRITE_BIT,
    .oldLayout        = old_layout,
    .newLayout        = new_layout,
    .image            = image,
    .subresourceRange = get_image_subresource_range(aspect_mask),
  };

  VkDependencyInfo dep_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers    = &barrier,
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

auto GraphicsEngine::get_image_subresource_range(VkImageAspectFlags aspect) -> VkImageSubresourceRange
{
  return
  {
    .aspectMask = aspect, 
    .levelCount = VK_REMAINING_MIP_LEVELS,
    .layerCount = VK_REMAINING_ARRAY_LAYERS,
  };
}

// HACK: VkCmdCopyImage can be faster but most restriction such as src and dst are same format and extent. 
void GraphicsEngine::copy_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D src_extent, VkExtent2D dst_extent)
{
  VkImageBlit2 blit
  { 
    .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
    .srcSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .dstSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
  };
  blit.srcOffsets[1].x = src_extent.width;
  blit.srcOffsets[1].y = src_extent.height;
  blit.srcOffsets[1].z = 1;
  blit.dstOffsets[1].x = dst_extent.width;
  blit.dstOffsets[1].y = dst_extent.height;
  blit.dstOffsets[1].z = 1;

  VkBlitImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
    .srcImage       = src,
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .dstImage       = dst,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &blit,
    .filter         = VK_FILTER_LINEAR,
  };

  vkCmdBlitImage2(cmd, &info);
}

void GraphicsEngine::copy_image(Command const& cmd, Image const& src, Image const& dst)
{
  VkImageCopy2 region
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
    .srcSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .dstSubresource =
    {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .layerCount = 1,
    },
    .extent = src.extent,
  };
  VkCopyImageInfo2 info
  {
    .sType          = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
    .srcImage       = src.handle,
    .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .dstImage       = dst.handle,
    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .regionCount    = 1,
    .pRegions       = &region,
  };
  vkCmdCopyImage2(cmd, &info);
}

void GraphicsEngine::depth_image_barrier_begin(VkCommandBuffer cmd)
{
  VkImageMemoryBarrier2 barrier
  {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask     = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT  | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    .srcAccessMask    = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstStageMask     = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT  | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    .dstAccessMask    = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .oldLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    .newLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    // HACK: currently, not use
    // .image            = _depth_image.image,
    .subresourceRange = 
    {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };

  VkDependencyInfo dep_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers    = &barrier,
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

void GraphicsEngine::depth_image_barrier_end(VkCommandBuffer cmd)
{
  VkImageMemoryBarrier2 barrier
  {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask     = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    .srcAccessMask    = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstStageMask     = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    .dstAccessMask    = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
    .oldLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    .newLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    // HACK: currently, not use
    // .image            = _depth_image.image,
    .subresourceRange = 
    {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .levelCount = 1,
      .layerCount = 1,
    },
  };

  VkDependencyInfo dep_info
  {
    .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers    = &barrier,
  };

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

} }
