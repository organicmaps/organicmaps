#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_staging_buffer.hpp"
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
namespace
{
uint32_t constexpr kDefaultStagingBufferSizeInBytes = 10 * 1024 * 1024;

VkImageMemoryBarrier PostRenderBarrier(VkImage image)
{
  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = nullptr;
  imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = 1;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = 1;
  return imageMemoryBarrier;
}
}  // namespace

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
  //TODO: do it for draw context only.
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

  m_defaultStagingBuffer.reset();

  for (auto & fbData : m_framebuffersData)
  {
    for (auto & framebuffer : fbData.second.m_framebuffers)
      vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    vkDestroyRenderPass(m_device, fbData.second.m_renderPass, nullptr);
  }

  DestroyDepthTexture();
  DestroySwapchain();
  DestroyCommandBuffers();
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

bool VulkanBaseContext::Validate()
{
  return true;
}

void VulkanBaseContext::Resize(int w, int h)
{
}

void VulkanBaseContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer)
{
  if (m_isActiveRenderPass)
  {
    if (m_currentFramebuffer != nullptr)
    {
      ref_ptr<Framebuffer> fb = m_currentFramebuffer;
      ref_ptr<VulkanTexture> tex = fb->GetTexture()->GetHardwareTexture();
      VkImageMemoryBarrier imageBarrier = PostRenderBarrier(tex->GetImage());
      vkCmdPipelineBarrier(m_renderingCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                           &imageBarrier);
    }

    vkCmdEndRenderPass(m_renderingCommandBuffer);
    m_isActiveRenderPass = false;

  }

  m_currentFramebuffer = framebuffer;
}

void VulkanBaseContext::ApplyFramebuffer(std::string const & framebufferLabel)
{
  vkCmdSetStencilReference(m_renderingCommandBuffer, VK_STENCIL_FRONT_AND_BACK,
                           m_stencilReferenceValue);

  auto & fbData = m_framebuffersData[m_currentFramebuffer];

  VkAttachmentLoadOp colorLoadOp, depthLoadOp, stencilLoadOp;
  VkAttachmentStoreOp colorStoreOp, depthStoreOp, stencilStoreOp;
  // Here, if we do not clear attachments, we load data ONLY if we store it afterwards, otherwise we use 'DontCare' option
  // to improve performance.
  if (m_clearBits & ClearBits::ColorBit)
    colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  else
    colorLoadOp = (m_storeBits & ClearBits::ColorBit) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  if (m_clearBits & ClearBits::DepthBit)
    depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  else
    depthLoadOp = (m_storeBits & ClearBits::DepthBit) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  if (m_clearBits & ClearBits::StencilBit)
    stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  else
    stencilLoadOp = (m_storeBits & ClearBits::StencilBit) ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;

  // Apply storing mode.
  if (m_storeBits & ClearBits::ColorBit)
    colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  else
    colorStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  if (m_storeBits & ClearBits::DepthBit)
    depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  else
    depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  if (m_storeBits & ClearBits::StencilBit)
    stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
  else
    stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  m_clearBits = 0;
  m_storeBits = 0;

  if (fbData.m_renderPass == VK_NULL_HANDLE)
  {
    VkFormat colorFormat = {};
    VkFormat depthFormat = {};

    if (m_currentFramebuffer == nullptr)
    {
      colorFormat = m_surfaceFormat.get().format;
      depthFormat = UnpackFormat(TextureFormat::Depth);

      fbData.m_renderPass = CreateRenderPass(2 /* attachmentsCount */,
                                             colorFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, colorStoreOp,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                             depthFormat, VK_ATTACHMENT_LOAD_OP_CLEAR, depthStoreOp,
                                             VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                             VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
    else
    {
      ref_ptr<dp::Framebuffer> framebuffer = m_currentFramebuffer;
      auto const depthStencilRef = framebuffer->GetDepthStencilRef();
      auto const attachmentsCount = (depthStencilRef != nullptr) ? 2 : 1;
      colorFormat = UnpackFormat(framebuffer->GetTexture()->GetFormat());
      if (depthStencilRef != nullptr)
        depthFormat = UnpackFormat(depthStencilRef->GetTexture()->GetFormat());

      fbData.m_renderPass = CreateRenderPass(attachmentsCount,
                                             colorFormat, colorLoadOp, colorStoreOp,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                             depthFormat, depthLoadOp, depthStoreOp,
                                             stencilLoadOp, stencilStoreOp,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
  }

  if (fbData.m_framebuffers.empty())
  {
    if (m_currentFramebuffer == nullptr)
    {
      std::array<VkImageView, 2> attachmentViews = {};

      // Depth/Stencil attachment is the same for all swapchain-bound frame buffers.
      attachmentViews[1] = m_depthStencil.m_imageView;

      VkFramebufferCreateInfo frameBufferCreateInfo = {};
      frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      frameBufferCreateInfo.pNext = nullptr;
      frameBufferCreateInfo.renderPass = fbData.m_renderPass;
      frameBufferCreateInfo.attachmentCount = 2;
      frameBufferCreateInfo.pAttachments = attachmentViews.data();
      frameBufferCreateInfo.width = m_surfaceCapabilities.currentExtent.width;
      frameBufferCreateInfo.height = m_surfaceCapabilities.currentExtent.height;
      frameBufferCreateInfo.layers = 1;

      // Create frame buffers for every swap chain image.
      fbData.m_framebuffers.resize(m_swapchainImageViews.size());
      for (size_t i = 0; i < fbData.m_framebuffers.size(); ++i)
      {
        attachmentViews[0] = m_swapchainImageViews[i];
        CHECK_VK_CALL(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr,
                                          &fbData.m_framebuffers[i]));
      }
    }
    else
    {
      ref_ptr<dp::Framebuffer> framebuffer = m_currentFramebuffer;

      auto const depthStencilRef = framebuffer->GetDepthStencilRef();
      auto const attachmentsCount = (depthStencilRef != nullptr) ? 2 : 1;
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
      frameBufferCreateInfo.renderPass = fbData.m_renderPass;
      frameBufferCreateInfo.attachmentCount = attachmentsCount;
      frameBufferCreateInfo.pAttachments = attachmentsViews.data();
      frameBufferCreateInfo.width = m_surfaceCapabilities.currentExtent.width;
      frameBufferCreateInfo.height = m_surfaceCapabilities.currentExtent.height;
      frameBufferCreateInfo.layers = 1;

      fbData.m_framebuffers.resize(1);
      CHECK_VK_CALL(vkCreateFramebuffer(m_device, &frameBufferCreateInfo, nullptr, &fbData.m_framebuffers[0]));
    }
  }

  m_pipelineKey.m_renderPass = fbData.m_renderPass;

  VkClearValue clearValues[2];
  clearValues[0].color = {{m_clearColor.GetRedF(), m_clearColor.GetGreenF(), m_clearColor.GetBlueF(),
                           m_clearColor.GetAlphaF()}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = fbData.m_renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent = m_surfaceCapabilities.currentExtent;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;
  renderPassBeginInfo.framebuffer = fbData.m_framebuffers[m_currentFramebuffer == nullptr ? m_imageIndex : 0];

  m_isActiveRenderPass = true;
  vkCmdBeginRenderPass(m_renderingCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanBaseContext::Init(ApiVersion apiVersion)
{
  m_defaultStagingBuffer = make_unique_dp<VulkanStagingBuffer>(m_objectManager,
                                                               kDefaultStagingBufferSizeInBytes);
}

void VulkanBaseContext::SetClearColor(Color const & color)
{
  m_clearColor = color;
}

void VulkanBaseContext::Clear(uint32_t clearBits, uint32_t storeBits)
{
  if (m_isActiveRenderPass)
  {
    VkClearRect clearRect = {};
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;
    clearRect.rect.extent = m_surfaceCapabilities.currentExtent;
    clearRect.rect.offset.x = 0;
    clearRect.rect.offset.y = 0;

    uint32_t constexpr kMaxClearAttachment = 2;
    std::array<VkClearAttachment, kMaxClearAttachment> attachments = {};
    uint32_t attachmentsCount = 0;
    {
      if (clearBits & ClearBits::ColorBit)
      {
        VkClearAttachment attachment = {};
        attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        attachment.colorAttachment = 0;
        attachment.clearValue.color = {{m_clearColor.GetRedF(), m_clearColor.GetGreenF(),
                                        m_clearColor.GetBlueF(), m_clearColor.GetAlphaF()}};
        CHECK_LESS(attachmentsCount, kMaxClearAttachment, ());
        attachments[attachmentsCount++] = std::move(attachment);
      }
      if (clearBits & ClearBits::DepthBit || clearBits & ClearBits::StencilBit)
      {
        VkClearAttachment attachment = {};
        if (clearBits & ClearBits::DepthBit)
          attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if (clearBits & ClearBits::StencilBit)
          attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        attachment.clearValue.depthStencil = {1.0f, 0};
        CHECK_LESS(attachmentsCount, kMaxClearAttachment, ());
        attachments[attachmentsCount++] = std::move(attachment);
      }
    }

    vkCmdClearAttachments(m_renderingCommandBuffer, attachmentsCount, attachments.data(),
                          1 /* rectCount */, &clearRect);
  }
  else
  {
    m_clearBits |= clearBits;
    m_storeBits |= storeBits;
  }
}

void VulkanBaseContext::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  VkViewport viewport = {};
  viewport.x = x;
  viewport.y = y;
  viewport.width = w;
  viewport.height = h;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(m_renderingCommandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.extent = {w, h};
  scissor.offset.x = x;
  scissor.offset.y = y;
  vkCmdSetScissor(m_renderingCommandBuffer, 0, 1, &scissor);
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
      DestroyCommandBuffers();
      DestroyCommandPool();
      DestroyDepthTexture();
    }
    m_surfaceFormat = surfaceFormat;
    m_surfaceCapabilities = surfaceCapabilities;
    CreateCommandPool();
    CreateCommandBuffers();
    CreateDepthTexture();
  }
  RecreateSwapchain();
}

void VulkanBaseContext::ResetSurface()
{
  vkDeviceWaitIdle(m_device);

  for (auto & framebuffer : m_framebuffersData[nullptr].m_framebuffers)
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  vkDestroyRenderPass(m_device, m_framebuffersData[nullptr].m_renderPass, nullptr);
  m_framebuffersData[nullptr] = {};
  // TODO(@darina): clear pipeline keys with the same renderPass
  DestroySwapchain();

  m_surface.reset();

  if (m_pipeline)
    m_pipeline->Dump(m_device);
}

void VulkanBaseContext::BeginRendering()
{
  // Record command buffer.
  // A fence is used to wait until this command buffer has finished execution and is no longer in-flight.
  // Command buffers can only be re-recorded or destroyed if they are not in-flight.
  CHECK_VK_CALL(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX));
  CHECK_VK_CALL(vkResetFences(m_device, 1, &m_fence));

  VkCommandBufferBeginInfo commandBufferBeginInfo = {};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CHECK_VK_CALL(vkBeginCommandBuffer(m_memoryCommandBuffer, &commandBufferBeginInfo));
  CHECK_VK_CALL(vkBeginCommandBuffer(m_renderingCommandBuffer, &commandBufferBeginInfo));

  // Prepare frame. Acquire next image.
  // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired
  // or an actual error is thrown. With that we don't have to handle VK_NOT_READY.
  VkResult res = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_presentComplete,
                                       (VkFence)nullptr, &m_imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
  {
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE)
    // or no longer optimal for presentation (SUBOPTIMAL).
    RecreateSwapchain();
  }
  else
  {
    CHECK_RESULT_VK_CALL(vkAcquireNextImageKHR, res);
  }
}

void VulkanBaseContext::Present()
{
  // Flushing of the default staging buffer must be before submitting the queue.
  // It guarantees the graphics data coherence.
  m_defaultStagingBuffer->Flush();

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PrePresent)])
    h.second(make_ref(this));

  CHECK(m_isActiveRenderPass, ());
  m_isActiveRenderPass = false;
  vkCmdEndRenderPass(m_renderingCommandBuffer);

  CHECK_VK_CALL(vkEndCommandBuffer(m_memoryCommandBuffer));
  CHECK_VK_CALL(vkEndCommandBuffer(m_renderingCommandBuffer));

  // Pipeline stage at which the queue submission will wait (via pWaitSemaphores).
  VkPipelineStageFlags const waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo = {};
  VkCommandBuffer commandBuffers[] = {m_memoryCommandBuffer, m_renderingCommandBuffer};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask = &waitStageMask;
  submitInfo.pWaitSemaphores = &m_presentComplete;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.commandBufferCount = 2;
  submitInfo.pCommandBuffers = commandBuffers;

  CHECK_VK_CALL(vkQueueSubmit(m_queue, 1, &submitInfo, m_fence));

  // Queue an image for presentation.
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = nullptr;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapchain;
  presentInfo.pImageIndices = &m_imageIndex;
  // Check if a wait semaphore has been specified to wait for before presenting the image.
  presentInfo.pWaitSemaphores = &m_renderComplete;
  presentInfo.waitSemaphoreCount = 1;

  VkResult res = vkQueuePresentKHR(m_queue, &presentInfo);
  if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
  {
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
      // Swap chain is no longer compatible with the surface and needs to be recreated.
      RecreateSwapchain();
    }
    else
    {
      CHECK_RESULT_VK_CALL(vkQueuePresentKHR, res);
    }
  }
  CHECK_VK_CALL(vkQueueWaitIdle(m_queue));

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PostPresent)])
    h.second(make_ref(this));

  // Resetting of the default staging buffer and collecting destroyed objects must be
  // only after the finishing of rendering. It prevents data collisions.
  m_defaultStagingBuffer->Reset();
  m_objectManager->CollectObjects();

  m_pipelineKey = {};
  m_stencilReferenceValue = 1;
  ClearParamDescriptors();
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

  // This mode waits for the vertical blank ("v-sync").
  swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

  swapchainCreateInfo.oldSwapchain = oldSwapchain;
  // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area.
  swapchainCreateInfo.clipped = VK_TRUE;

  CHECK_VK_CALL(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

  if (oldSwapchain != VK_NULL_HANDLE)
  {
    for (auto const & imageView : m_swapchainImageViews)
      vkDestroyImageView(m_device, imageView, nullptr);
    m_swapchainImageViews.clear();
  }

  // Create swapchain image views.
  uint32_t swapchainImageCount = 0;
  CHECK_VK_CALL(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr));

  m_swapchainImages.resize(swapchainImageCount);
  CHECK_VK_CALL(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_swapchainImages.data()));

  m_swapchainImageViews.resize(swapchainImageCount);
  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
  {
    VkImageViewCreateInfo colorAttachmentImageView = {};
    colorAttachmentImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorAttachmentImageView.image = m_swapchainImages[i];
    colorAttachmentImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorAttachmentImageView.format = m_surfaceFormat.get().format;
    colorAttachmentImageView.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                           VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    colorAttachmentImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentImageView.subresourceRange.baseMipLevel = 0;
    colorAttachmentImageView.subresourceRange.levelCount = 1;
    colorAttachmentImageView.subresourceRange.baseArrayLayer = 0;
    colorAttachmentImageView.subresourceRange.layerCount = 1;
    CHECK_VK_CALL(vkCreateImageView(m_device, &colorAttachmentImageView, nullptr,
                                    &m_swapchainImageViews[i]));
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
  // This flag will implicitly reset command buffers from this pool when calling vkBeginCommandBuffer.
  commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolCI.queueFamilyIndex = m_renderingQueueFamilyIndex;
  CHECK_VK_CALL(vkCreateCommandPool(m_device, &commandPoolCI, nullptr, &m_commandPool));
}

void VulkanBaseContext::DestroyCommandPool()
{
  vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

void VulkanBaseContext::CreateCommandBuffers()
{
  // A fence is need to check for command buffer completion before we can recreate it.
  VkFenceCreateInfo fenceCI = {};
  fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  CHECK_VK_CALL(vkCreateFence(m_device, &fenceCI, nullptr, &m_fence));

  // Semaphores are used to order queue submissions.
  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  CHECK_VK_CALL(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_presentComplete));
  CHECK_VK_CALL(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_renderComplete));

  // Create a single command buffer that is recorded every frame.
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = m_commandPool;
  cmdBufAllocateInfo.commandBufferCount = 1;
  cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CHECK_VK_CALL(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &m_memoryCommandBuffer));
  CHECK_VK_CALL(vkAllocateCommandBuffers(m_device, &cmdBufAllocateInfo, &m_renderingCommandBuffer));
}

void VulkanBaseContext::DestroyCommandBuffers()
{
  vkDestroyFence(m_device, m_fence, nullptr);
  vkDestroySemaphore(m_device, m_presentComplete, nullptr);
  vkDestroySemaphore(m_device, m_renderComplete, nullptr);
  vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_memoryCommandBuffer);
  vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_renderingCommandBuffer);
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

VkRenderPass VulkanBaseContext::CreateRenderPass(uint32_t attachmentsCount, VkFormat colorFormat,
                                                 VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                                                 VkImageLayout initLayout, VkImageLayout finalLayout,
                                                 VkFormat depthFormat, VkAttachmentLoadOp depthLoadOp,
                                                 VkAttachmentStoreOp depthStoreOp, VkAttachmentLoadOp stencilLoadOp,
                                                 VkAttachmentStoreOp stencilStoreOp, VkImageLayout depthInitLayout,
                                                 VkImageLayout depthFinalLayout)
{
  std::vector<VkAttachmentDescription> attachments(attachmentsCount);

  // Color attachment.
  attachments[0].format = colorFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = loadOp;
  attachments[0].storeOp = storeOp;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = initLayout;
  attachments[0].finalLayout = finalLayout;

  if (attachmentsCount == 2)
  {
    // Depth attachment.
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = depthLoadOp;
    attachments[1].storeOp = depthStoreOp;
    attachments[1].stencilLoadOp = stencilLoadOp;
    attachments[1].stencilStoreOp = stencilStoreOp;
    attachments[1].initialLayout = depthInitLayout;
    attachments[1].finalLayout = depthFinalLayout;
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

  // Subpass dependencies for layout transitions.
  std::array<VkSubpassDependency, 2> dependencies = {};

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
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachmentsCount;
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VkRenderPass renderPass = {};
  CHECK_VK_CALL(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass));

  return renderPass;
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
  if (descriptor.m_type == ParamDescriptor::Type::DynamicUniformBuffer)
  {
    for (auto & param : m_paramDescriptors)
    {
      if (param.m_type == ParamDescriptor::Type::DynamicUniformBuffer)
      {
        param = std::move(descriptor);
        return;
      }
    }
  }
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

ref_ptr<VulkanStagingBuffer> VulkanBaseContext::GetDefaultStagingBuffer() const
{
  return make_ref(m_defaultStagingBuffer);
}
}  // namespace vulkan
}  // namespace dp
