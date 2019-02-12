#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_texture.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "drape/framebuffer.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace dp
{
namespace vulkan
{
VulkanBaseContext::VulkanBaseContext(VkInstance vulkanInstance, VkPhysicalDevice gpu,
                                     VkPhysicalDeviceProperties const & gpuProperties,
                                     VkDevice device, uint32_t renderingQueueFamilyIndex,
                                     ref_ptr<VulkanObjectManager> objectManager,
                                     drape_ptr<VulkanPipeline> && pipeline)
  : m_vulkanInstance(vulkanInstance)
  , m_gpu(gpu)
  , m_gpuProperties(gpuProperties)
  , m_device(device)
  , m_renderingQueueFamilyIndex(renderingQueueFamilyIndex)
  , m_objectManager(std::move(objectManager))
  , m_pipeline(std::move(pipeline))
{
  // Get a graphics queue from the device
  vkGetDeviceQueue(m_device, m_renderingQueueFamilyIndex, 0, &m_queue);
}

VulkanBaseContext::~VulkanBaseContext()
{
  if (m_pipeline)
  {
    m_pipeline->Destroy(m_device);
    m_pipeline.reset();
  }

  DestroyDefaultFramebuffer();
  DestroyDepthTexture();
  DestroySwapchain();
  DestroyRenderPass();
  DestroyCommandBuffer();
  DestroyCommandPool();
}

std::string VulkanBaseContext::GetRendererName() const
{
  return m_gpuProperties.deviceName;
}

std::string VulkanBaseContext::GetRendererVersion() const
{
  std::ostringstream ss;
  ss << "API:" << VK_VERSION_MAJOR(m_gpuProperties.apiVersion) << "."
     << VK_VERSION_MINOR(m_gpuProperties.apiVersion) << "."
     << VK_VERSION_PATCH(m_gpuProperties.apiVersion)
     << "/Driver:" << VK_VERSION_MAJOR(m_gpuProperties.driverVersion) << "."
     << VK_VERSION_MINOR(m_gpuProperties.driverVersion) << "."
     << VK_VERSION_PATCH(m_gpuProperties.driverVersion);
  return ss.str();
}

void VulkanBaseContext::MakeCurrent()
{

}

void VulkanBaseContext::DoneCurrent()
{

}

bool VulkanBaseContext::Validate()
{
  return true;
}

void VulkanBaseContext::Resize(int w, int h)
{
}

void VulkanBaseContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer)
{
  if (m_renderPass != VK_NULL_HANDLE)
  {
    vkCmdEndRenderPass(m_commandBuffer);
    DestroyRenderPass();
  }

  if (m_framebuffer != VK_NULL_HANDLE)
    DestroyFramebuffer();

  m_currentFramebuffer = framebuffer;
}

void VulkanBaseContext::ApplyFramebuffer(std::string const & framebufferLabel)
{
  //TODO: set renderPass to m_pipelineKey
  vkCmdSetStencilReference(m_commandBuffer, VK_STENCIL_FRONT_AND_BACK, m_stencilReferenceValue);

  CreateRenderPass();

  VkClearValue clearValues[2];
  clearValues[0].color = {1.0f, 0.0f, 1.0f, 1.0f};
  clearValues[1].depthStencil = { 1.0f, m_stencilReferenceValue };

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = m_renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent = m_surfaceCapabilities.currentExtent;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;
  if (m_currentFramebuffer != nullptr)
  {
    CreateFramebuffer();
    renderPassBeginInfo.framebuffer = m_framebuffer;
  }
  else
  {
    if (m_defaultFramebuffers.empty())
      CreateDefaultFramebuffer();
    renderPassBeginInfo.framebuffer = m_defaultFramebuffers[m_imageIndex];
  }
  vkCmdBeginRenderPass(m_commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanBaseContext::Init(ApiVersion apiVersion)
{
}

void VulkanBaseContext::SetClearColor(Color const & color)
{

}

void VulkanBaseContext::Clear(uint32_t clearBits, uint32_t storeBits)
{
 //vkCmdClearColorImage();
}

void VulkanBaseContext::Flush()
{

}

void VulkanBaseContext::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  VkViewport viewport;
  viewport.width = w;
  viewport.height = h;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent = {w, h};
  scissor.offset.x = x;
  scissor.offset.y = y;
  vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);
}

void VulkanBaseContext::SetSurface(VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat,
                                   VkSurfaceCapabilitiesKHR surfaceCapabilities, int width, int height)
{
  m_surface = surface;
  if (!m_surfaceFormat.is_initialized() ||
    m_surfaceFormat.get().format != surfaceFormat.format ||
    m_surfaceFormat.get().colorSpace != surfaceFormat.colorSpace)
  {
    if (m_surfaceFormat.is_initialized())
    {
      DestroyRenderPass();
      DestroyCommandBuffer();
      DestroyCommandPool();
      DestroyDepthTexture();
    }
    m_surfaceFormat = surfaceFormat;
    m_surfaceCapabilities = surfaceCapabilities;
    CreateCommandPool();
    CreateCommandBuffer();
    CreateDepthTexture();
  }
  RecreateSwapchain();
}

void VulkanBaseContext::ResetSurface()
{
  vkDeviceWaitIdle(m_device);
  DestroyDefaultFramebuffer();
  m_surface.reset();

  if (m_pipeline)
    m_pipeline->Dump(m_device);
}

void VulkanBaseContext::BeginRendering()
{
  // Record command buffer.
  // A fence is used to wait until this command buffer has finished execution and is no longer in-flight
  // Command buffers can only be re-recorded or destroyed if they are not in-flight
  CHECK_VK_CALL(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX));
  CHECK_VK_CALL(vkResetFences(m_device, 1, &m_fence));

  VkCommandBufferBeginInfo commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CHECK_VK_CALL(vkBeginCommandBuffer(m_commandBuffer, &commandBufferBeginInfo));

  // Prepare frame. Acquire next image.
  // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual
  // error is thrown. With that we don't have to handle VK_NOT_READY
  VkResult res = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_presentComplete,
                                       (VkFence)nullptr, &m_imageIndex); //???????????????????????????????????
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
    // or no longer optimal for presentation (SUBOPTIMAL)
    RecreateSwapchain();
  }
  else
  {
    CHECK_RESULT_VK_CALL(vkAcquireNextImageKHR, res);
  }
}

void VulkanBaseContext::Present()
{
  // Resetting of the default staging buffer must be before submitting the queue.
  // It guarantees the graphics data coherence.
  m_objectManager->FlushDefaultStagingBuffer();

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PrePresent)])
    h.second(make_ref(this));

  // TODO: wait for all map-memory operations.

  if (m_renderPass != VK_NULL_HANDLE)
  {
    vkCmdEndRenderPass(m_commandBuffer);
    DestroyRenderPass();
  }

  CHECK_VK_CALL(vkEndCommandBuffer(m_commandBuffer));

  // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
  const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask = &waitStageMask;
  submitInfo.pWaitSemaphores = &m_presentComplete;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffer;

  CHECK_VK_CALL(vkQueueSubmit(m_queue, 1, &submitInfo, m_fence));
  //CHECK_VK_CALL(vkQueueWaitIdle(m_queue));

  // Queue an image for presentation.
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapchain;
  presentInfo.pImageIndices = &m_imageIndex;
  // Check if a wait semaphore has been specified to wait for before presenting the image
  presentInfo.pWaitSemaphores = &m_renderComplete;
  presentInfo.waitSemaphoreCount = 1;

  VkResult res = vkQueuePresentKHR(m_queue, &presentInfo);
  if (!(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR))
  {
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
      // Swap chain is no longer compatible with the surface and needs to be recreated
      RecreateSwapchain();
    }
    else
    {
      CHECK_RESULT_VK_CALL(vkQueuePresentKHR, res);
    }
  }
  else
  {
    CHECK_VK_CALL(vkQueueWaitIdle(m_queue));
  }

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PostPresent)])
    h.second(make_ref(this));

  // Resetting of the default staging buffer and collecting destroyed objects must be
  // only after the finishing of rendering. It prevents data collisions.
  m_objectManager->ResetDefaultStagingBuffer();
  m_objectManager->CollectObjects();

  m_pipelineKey = {};
  m_stencilReferenceValue = 1;
  ClearParamDescriptors();

  for (auto framebuffer : m_framebuffersToDestroy)
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  m_framebuffersToDestroy.clear();

  for (auto renderPass : m_renderPassesToDestroy)
    vkDestroyRenderPass(m_device, renderPass, nullptr);
  m_renderPassesToDestroy.clear();
}

uint32_t VulkanBaseContext::RegisterHandler(HandlerType handlerType, ContextHandler && handler)
{
  static uint32_t counter = 0;
  CHECK_LESS(counter, std::numeric_limits<uint32_t>::max(), ());
  ASSERT(handler != nullptr, ());
  uint32_t const id = ++counter;
  m_handlers[static_cast<uint32_t>(handlerType)].emplace_back(std::make_pair(id, std::move(handler)));
  return id;
}

void VulkanBaseContext::UnregisterHandler(uint32_t id)
{
  for (size_t i = 0; i < m_handlers.size(); ++i)
  {
    m_handlers[i].erase(std::remove_if(m_handlers[i].begin(), m_handlers[i].end(),
                                       [id](std::pair<uint8_t, ContextHandler> const & p)
    {
      return p.first == id;
    }), m_handlers[i].end());
  }
}

void VulkanBaseContext::RecreateSwapchain()
{
  CHECK(m_surface.is_initialized(), ());
  CHECK(m_surfaceFormat.is_initialized(), ());

  VkSwapchainKHR oldSwapchain = m_swapchain;

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
  swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreateInfo.pNext = nullptr;
  swapchainCreateInfo.surface = m_surface.get();
  swapchainCreateInfo.minImageCount = std::min(m_surfaceCapabilities.minImageCount + 1,
                                               m_surfaceCapabilities.maxImageCount);
  swapchainCreateInfo.imageFormat = m_surfaceFormat.get().format;
  swapchainCreateInfo.imageColorSpace = m_surfaceFormat.get().colorSpace;
  swapchainCreateInfo.imageExtent = m_surfaceCapabilities.currentExtent;

  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (m_surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  if (m_surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  CHECK(m_surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, ());
  swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.queueFamilyIndexCount = 0;
  swapchainCreateInfo.pQueueFamilyIndices = nullptr;

  CHECK(m_surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, ());
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

  // This mode waits for the vertical blank ("v-sync")
  swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

  swapchainCreateInfo.oldSwapchain = oldSwapchain;
  // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
  swapchainCreateInfo.clipped = VK_TRUE;

  CHECK_VK_CALL(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

  if (oldSwapchain != VK_NULL_HANDLE)
  {
    for (auto const & imageView : m_swapchainImageViews)
      vkDestroyImageView(m_device, imageView, nullptr);
    m_swapchainImageViews.clear();
    vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
  }

  // Create swapchain image views
  uint32_t swapchainImageCount = 0;
  CHECK_VK_CALL(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr));

  std::vector<VkImage> swapchainImages(swapchainImageCount);
  CHECK_VK_CALL(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, swapchainImages.data()));

  m_swapchainImageViews.resize(swapchainImages.size());
  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
  {
    VkImageViewCreateInfo colorAttachmentImageView = {};
    colorAttachmentImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorAttachmentImageView.image = swapchainImages[i];
    colorAttachmentImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorAttachmentImageView.format = m_surfaceFormat.get().format;
    colorAttachmentImageView.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorAttachmentImageView.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorAttachmentImageView.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorAttachmentImageView.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    colorAttachmentImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentImageView.subresourceRange.baseMipLevel = 0;
    colorAttachmentImageView.subresourceRange.levelCount = 1;
    colorAttachmentImageView.subresourceRange.baseArrayLayer = 0;
    colorAttachmentImageView.subresourceRange.layerCount = 1;
    CHECK_VK_CALL(vkCreateImageView(m_device, &colorAttachmentImageView, nullptr, &m_swapchainImageViews[i]));
  }
}

void VulkanBaseContext::DestroySwapchain()
{
  for (auto const & imageView : m_swapchainImageViews)
    vkDestroyImageView(m_device, imageView, nullptr);
  m_swapchainImageViews.clear();
  vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
  m_swapchain = VK_NULL_HANDLE;
}

void VulkanBaseContext::CreateCommandPool()
{
  VkCommandPoolCreateInfo commandPoolCI = {};
  commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  // This flag will implicitly reset command buffers from this pool when calling vkBeginCommandBuffer
  commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolCI.queueFamilyIndex = m_renderingQueueFamilyIndex;
  CHECK_VK_CALL(vkCreateCommandPool(m_device, &commandPoolCI, nullptr, &m_commandPool));
}

void VulkanBaseContext::DestroyCommandPool()
{
  vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

void VulkanBaseContext::CreateCommandBuffer()
{
  // A fence is need to check for command buffer completion before we can recreate it
  VkFenceCreateInfo fenceCI = {};
  fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  CHECK_VK_CALL(vkCreateFence(m_device, &fenceCI, nullptr, &m_fence));

  // Semaphores are used to order queue submissions
  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  CHECK_VK_CALL(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_presentComplete));
  CHECK_VK_CALL(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_renderComplete));

  // Create a single command buffer that is recorded every frame
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = m_commandPool;
  cmdBufAllocateInfo.commandBufferCount = 1;
  cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CHECK_VK_CALL(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &m_commandBuffer));
}

void VulkanBaseContext::DestroyCommandBuffer()
{
  vkDestroyFence(m_device, m_fence, nullptr);
  vkDestroySemaphore(m_device, m_presentComplete, nullptr);
  vkDestroySemaphore(m_device, m_renderComplete, nullptr);
  vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
}

void VulkanBaseContext::CreateDepthTexture()
{
  CHECK(m_depthStencil.m_image == VK_NULL_HANDLE, ());
  m_depthStencil = m_objectManager->CreateImage(
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    UnpackFormat(TextureFormat::Depth),
    VK_IMAGE_ASPECT_DEPTH_BIT,
    m_surfaceCapabilities.currentExtent.width, m_surfaceCapabilities.currentExtent.height);
}

void VulkanBaseContext::DestroyDepthTexture()
{
  if (m_depthStencil.m_image != VK_NULL_HANDLE)
    m_objectManager->DestroyObject(m_depthStencil);
}

void VulkanBaseContext::CreateDefaultFramebuffer()
{
  std::array<VkImageView, 2> attachments = {};

  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = m_depthStencil.m_imageView;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = nullptr;
  frameBufferCreateInfo.renderPass = m_renderPass;
  frameBufferCreateInfo.attachmentCount = 2;
  frameBufferCreateInfo.pAttachments = attachments.data();
  frameBufferCreateInfo.width = m_surfaceCapabilities.currentExtent.width;
  frameBufferCreateInfo.height = m_surfaceCapabilities.currentExtent.height;
  frameBufferCreateInfo.layers = 1;

  // Create frame buffers for every swap chain image
  m_defaultFramebuffers.resize(m_swapchainImageViews.size());
  for (uint32_t i = 0; i < m_defaultFramebuffers.size(); i++)
  {
    attachments[0] = m_swapchainImageViews[i];
    CHECK_VK_CALL(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_defaultFramebuffers[i]));
  }
}

void VulkanBaseContext::DestroyDefaultFramebuffer()
{
  for (uint32_t i = 0; i < m_defaultFramebuffers.size(); i++)
  {
    vkDestroyFramebuffer(m_device, m_defaultFramebuffers[i], nullptr);
  }
  m_defaultFramebuffers.clear();
}

void VulkanBaseContext::CreateFramebuffer()
{
  ref_ptr<dp::Framebuffer> framebuffer = m_currentFramebuffer;

  auto const depthStencilRef = framebuffer->GetDepthStencilRef();
  size_t const attachmentsCount = depthStencilRef != nullptr ? 2 : 1;
  std::vector<VkImageView> attachmentsViews(attachmentsCount);

  ASSERT(dynamic_cast<VulkanTexture *>(framebuffer->GetTexture()->GetHardwareTexture().get()) != nullptr, ());
  ref_ptr<VulkanTexture> colorAttachment = framebuffer->GetTexture()->GetHardwareTexture();

  attachmentsViews[0] = colorAttachment->GetTextureView();

  if (depthStencilRef != nullptr)
  {
    ASSERT(dynamic_cast<VulkanTexture *>(depthStencilRef->GetTexture()->GetHardwareTexture().get()) != nullptr, ());
    ref_ptr<VulkanTexture> depthStencilAttachment = depthStencilRef->GetTexture()->GetHardwareTexture();
    attachmentsViews[1] = depthStencilAttachment->GetTextureView();
  }

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = nullptr;
  frameBufferCreateInfo.renderPass = m_renderPass;
  frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentsCount);
  frameBufferCreateInfo.pAttachments = attachmentsViews.data();
  frameBufferCreateInfo.width = m_surfaceCapabilities.currentExtent.width;
  frameBufferCreateInfo.height = m_surfaceCapabilities.currentExtent.height;
  frameBufferCreateInfo.layers = 1;

  CHECK_VK_CALL(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &m_framebuffer));
}

void VulkanBaseContext::DestroyFramebuffer()
{
  CHECK(m_framebuffer != VK_NULL_HANDLE, ());
  m_framebuffersToDestroy.push_back(m_framebuffer);
  m_framebuffer = VK_NULL_HANDLE;
}

void VulkanBaseContext::CreateRenderPass()
{
  VkFormat colorFormat = m_surfaceFormat.get().format;
  VkFormat depthFormat = UnpackFormat(TextureFormat::Depth);
  if (m_currentFramebuffer != nullptr)
  {
    ref_ptr<dp::Framebuffer> framebuffer = m_currentFramebuffer;

    colorFormat = UnpackFormat(framebuffer->GetTexture()->GetFormat());

    auto const depthStencilRef = framebuffer->GetDepthStencilRef();
    if (depthStencilRef != nullptr)
    {
      depthFormat = UnpackFormat(depthStencilRef->GetTexture()->GetFormat());
    }
    else
    {
      depthFormat = VK_FORMAT_UNDEFINED;
    }
  }
  size_t const attachmentsCount = depthFormat == VK_FORMAT_UNDEFINED ? 1 : 2;

  std::vector<VkAttachmentDescription> attachments(attachmentsCount);

  // Color attachment
  attachments[0].format = colorFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = m_currentFramebuffer != nullptr ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                               : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  if (attachmentsCount == 2)
  {
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = attachmentsCount == 2 ? &depthReference : nullptr;
  subpassDescription.inputAttachmentCount = 0;
  subpassDescription.pInputAttachments = nullptr;
  subpassDescription.preserveAttachmentCount = 0;
  subpassDescription.pPreserveAttachments = nullptr;
  subpassDescription.pResolveAttachments = nullptr;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; //??????????????????????
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  CHECK_VK_CALL(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));
}

void VulkanBaseContext::DestroyRenderPass()
{
  CHECK(m_renderPass != VK_NULL_HANDLE, ());
  m_renderPassesToDestroy.push_back(m_renderPass);
  m_renderPass = {};
}

void VulkanBaseContext::SetDepthTestEnabled(bool enabled)
{
  m_pipelineKey.m_depthStencil.SetDepthTestEnabled(enabled);
}

void VulkanBaseContext::SetDepthTestFunction(TestFunction depthFunction)
{
  m_pipelineKey.m_depthStencil.SetDepthTestFunction(depthFunction);
}

void VulkanBaseContext::SetStencilTestEnabled(bool enabled)
{
  m_pipelineKey.m_depthStencil.SetStencilTestEnabled(enabled);
}

void VulkanBaseContext::SetStencilFunction(StencilFace face, TestFunction stencilFunction)
{
  m_pipelineKey.m_depthStencil.SetStencilFunction(face, stencilFunction);
}

void VulkanBaseContext::SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                                          StencilAction depthFailAction, StencilAction passAction)
{
  m_pipelineKey.m_depthStencil.SetStencilActions(face, stencilFailAction, depthFailAction, passAction);
}

void VulkanBaseContext::SetStencilReferenceValue(uint32_t stencilReferenceValue)
{
  m_stencilReferenceValue = stencilReferenceValue;
}
    
void VulkanBaseContext::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
  m_pipelineKey.m_primitiveTopology = topology;
}
    
void VulkanBaseContext::SetBindingInfo(std::vector<dp::BindingInfo> const & bindingInfo)
{
  m_pipelineKey.m_bindingInfo = bindingInfo;
}

void VulkanBaseContext::SetProgram(ref_ptr<VulkanGpuProgram> program)
{
  m_pipelineKey.m_program = program;
}
    
void VulkanBaseContext::SetBlendingEnabled(bool blendingEnabled)
{
  m_pipelineKey.m_blendingEnabled = blendingEnabled;
}
    
void VulkanBaseContext::ApplyParamDescriptor(ParamDescriptor && descriptor)
{
  m_paramDescriptors.push_back(std::move(descriptor));
}
    
void VulkanBaseContext::ClearParamDescriptors()
{
  m_paramDescriptors.clear();
}

VkPipeline VulkanBaseContext::GetCurrentPipeline()
{
  return m_pipeline->GetPipeline(m_device, m_pipelineKey);
}

DescriptorSetGroup VulkanBaseContext::GetCurrentDescriptorSetGroup()
{
  CHECK(m_pipelineKey.m_program != nullptr, ());
  CHECK(!m_paramDescriptors.empty(), ("Shaders parameters are not set."));
  return m_objectManager->CreateDescriptorSetGroup(m_pipelineKey.m_program, m_paramDescriptors);
}

VkPipelineLayout VulkanBaseContext::GetCurrentPipelineLayout() const
{
  CHECK(m_pipelineKey.m_program != nullptr, ());
  return m_pipelineKey.m_program->GetPipelineLayout();
}

uint32_t VulkanBaseContext::GetCurrentDynamicBufferOffset() const
{
  for (auto const & p : m_paramDescriptors)
  {
    if (p.m_type == ParamDescriptor::Type::DynamicUniformBuffer)
      return p.m_bufferDynamicOffset;
  }
  CHECK(false, ("Shaders parameters are not set."));
  return 0;
}

VkSampler VulkanBaseContext::GetSampler(SamplerKey const & key)
{
  return m_objectManager->GetSampler(key);
}
}  // namespace vulkan
}  // namespace dp
