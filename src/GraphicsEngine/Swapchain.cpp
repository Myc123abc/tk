#include "Swapchain.hpp"
#include "init-util.hpp"


namespace tk { namespace graphics_engine {

void Swapchain::init(VkPhysicalDevice physical_device, VkDevice device, VkSurfaceKHR surface, Window const* window)
{
  _physical_device = physical_device;
  _surface         = surface;
  _device          = device;
  _window          = window;

  auto     details        = SwapchainDetails(physical_device, surface);
  auto     surface_format = details.get_surface_format();
  auto     present_mode   = details.get_present_mode();
  auto     extent         = details.get_swap_extent(window);
  uint32_t image_count    = details.capabilities.minImageCount + 1;

  if (details.capabilities.maxImageCount > 0 &&
      image_count > details.capabilities.maxImageCount)
    image_count = details.capabilities.maxImageCount;

  _swapchain_create_info =
  {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = surface,
    .minImageCount    = image_count,
    .imageFormat      = surface_format.format,
    .imageColorSpace  = surface_format.colorSpace,
    .imageExtent      = extent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .preTransform     = details.capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = present_mode,
    .clipped          = VK_TRUE,
  };

  auto queue_families = get_queue_family_indices(physical_device, surface);
  uint32_t indices[]
  {
    queue_families.graphics_family.value(),
    queue_families.present_family.value(),
  };

  if (queue_families.graphics_family != queue_families.present_family)
  {
    _swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    _swapchain_create_info.queueFamilyIndexCount = 2;
    _swapchain_create_info.pQueueFamilyIndices   = indices;
  }
  else
    _swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;

  throw_if(vkCreateSwapchainKHR(device, &_swapchain_create_info, nullptr, &_swapchain) != VK_SUCCESS,
           "failed to create swapchain");

  set_images();
}

void Swapchain::set_images()
{
  uint32_t image_count;
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);

  // get swapchain images
  auto images = std::vector<VkImage>(image_count);
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, images.data());

  // create image views
  auto image_views = std::vector<VkImageView>(image_count);
  for (auto i = 0; i < image_count; ++i)
  {
    VkImageViewCreateInfo info
    {
      .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image    = images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format   = _swapchain_create_info.imageFormat,
      .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
      },
    };
    vkCreateImageView(_device, &info, nullptr, &image_views[i]);
  }

  // set image info to _swapchain_images
  _images.reserve(image_count);
  for (auto i = 0; i < image_count; ++i)
    _images.emplace_back(images[i], image_views[i], _swapchain_create_info.imageExtent, _swapchain_create_info.imageFormat);
}

void Swapchain::destroy()
{
  vkDestroySwapchainKHR(_device, _swapchain, nullptr);
  for (auto const& image : _images)
    vkDestroyImageView(_device, image.view(), nullptr);
}

void Swapchain::resize()
{
  vkDeviceWaitIdle(_device);

  _swapchain_create_info.imageExtent  = SwapchainDetails(_physical_device, _surface).get_swap_extent(_window);
  _swapchain_create_info.oldSwapchain = _swapchain;
  throw_if(vkCreateSwapchainKHR(_device, &_swapchain_create_info, nullptr, &_swapchain) != VK_SUCCESS,
      "failed to create swapchain");
  vkDestroySwapchainKHR(_device, _swapchain_create_info.oldSwapchain, nullptr);

  for (auto const& image : _images)
    vkDestroyImageView(_device, image.view(), nullptr);
  _images.clear();

  set_images();
}

}}