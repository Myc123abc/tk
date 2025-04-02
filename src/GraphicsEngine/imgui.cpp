#include "GraphicsEngine.hpp"
#include "init-util.hpp"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace tk { namespace graphics_engine {

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void GraphicsEngine::init_imgui()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplSDL3_InitForVulkan(_window.get());
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = _instance;
  init_info.PhysicalDevice = _physical_device;
  init_info.Device = _device;
  auto queue_family = get_queue_family_indices(_physical_device, _surface);
  init_info.QueueFamily = queue_family.graphics_family.value();
  init_info.Queue = _graphics_queue;
  init_info.PipelineCache = nullptr;

  VkDescriptorPoolSize pool_sizes[] =
  {
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
  };
  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 0;
  for (VkDescriptorPoolSize& pool_size : pool_sizes)
      pool_info.maxSets += pool_size.descriptorCount;
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;
  throw_if(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imgui_descriptor_pool) != VK_SUCCESS,
           "failed to create imgui descriptor pool");

  init_info.DescriptorPool = _imgui_descriptor_pool;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = check_vk_result;

  VkAttachmentDescription attachment = {};
  attachment.format = _image.format;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  info.subpassCount = 1;
  info.dependencyCount = 1;
  throw_if(vkCreateRenderPass(_device, &info, nullptr, &_imgui_render_pass) != VK_SUCCESS,
           "failed to create imgui render pass");
  init_info.RenderPass = _imgui_render_pass;

  ImGui_ImplVulkan_Init(&init_info);
  // // (this gets a bit more complicated, see example app for full reference)
  // ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
  // // (your code submit a queue)
  // ImGui_ImplVulkan_DestroyFontUploadObjects();
}

} }
