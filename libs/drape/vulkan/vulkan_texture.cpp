#include "drape/vulkan/vulkan_texture.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

drape_ptr<dp::HWTextureAllocator> CreateVulkanAllocator()
{
  return make_unique_dp<dp::vulkan::VulkanTextureAllocator>();
}

ref_ptr<dp::HWTextureAllocator> GetDefaultVulkanAllocator()
{
  static dp::vulkan::VulkanTextureAllocator allocator;
  return make_ref(&allocator);
}

namespace dp
{
namespace vulkan
{
namespace
{
VkBufferImageCopy BufferCopyRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stagingOffset)
{
  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = 0;
  bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferCopyRegion.imageSubresource.layerCount = 1;
  bufferCopyRegion.imageExtent.width = width;
  bufferCopyRegion.imageExtent.height = height;
  bufferCopyRegion.imageExtent.depth = 1;
  bufferCopyRegion.imageOffset.x = x;
  bufferCopyRegion.imageOffset.y = y;
  bufferCopyRegion.imageOffset.z = 0;
  bufferCopyRegion.bufferOffset = stagingOffset;
  return bufferCopyRegion;
}
}  // namespace

drape_ptr<HWTexture> VulkanTextureAllocator::CreateTexture(ref_ptr<dp::GraphicsContext> context)
{
  return make_unique_dp<VulkanTexture>(make_ref(this));
}

VulkanTexture::~VulkanTexture()
{
  m_objectManager->DestroyObject(m_textureObject);

  std::lock_guard<std::mutex> lock(m_dedicatedStagingBufferMutex);
  m_objectManager->DestroyObject(m_dedicatedStagingBuffer);
  m_dedicatedStagingBuffer = {};
}

void VulkanTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  Base::Create(context, params, data);

  static uint32_t textureCounter = 0;
  textureCounter++;
  m_textureID = textureCounter;

  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  m_objectManager = vulkanContext->GetObjectManager();

  if (Validate())
  {
    m_objectManager->DestroyObject(m_textureObject);
    m_textureObject = {};
  }

  auto const format = VulkanFormatUnpacker::Unpack(params.m_format);

  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(vulkanContext->GetPhysicalDevice(), format, &formatProperties);
  VkImageTiling tiling = VK_IMAGE_TILING_LINEAR;
  if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
    tiling = VK_IMAGE_TILING_OPTIMAL;

  m_isMutable = params.m_isMutable;
  if (params.m_isRenderTarget)
  {
    // Create image.
    if (params.m_format == TextureFormat::DepthStencil || params.m_format == TextureFormat::Depth)
    {
      m_aspectFlags = params.m_format == TextureFormat::DepthStencil
                        ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
                        : VK_IMAGE_ASPECT_DEPTH_BIT;
      m_textureObject = m_objectManager->CreateImage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, format, tiling,
                                                     m_aspectFlags, params.m_width, params.m_height);
    }
    else
    {
      m_aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
      m_textureObject = m_objectManager->CreateImage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                     format, tiling, m_aspectFlags, params.m_width, params.m_height);
    }
  }
  else
  {
    if (data != nullptr || params.m_usePersistentStagingBuffer)
    {
      std::lock_guard<std::mutex> lock(m_dedicatedStagingBufferMutex);

      if (m_dedicatedStagingBuffer.m_buffer)
        m_objectManager->DestroyObject(m_dedicatedStagingBuffer);

      // Create dedicated staging buffer.
      auto const notAlignedSize = GetBytesPerPixel(params.m_format) * params.m_width * params.m_height;
      auto bufferSize = VulkanMemoryManager::GetAligned(notAlignedSize, 64);

      auto constexpr kStagingBuffer = VulkanMemoryManager::ResourceType::Staging;
      VkDevice device = m_objectManager->GetDevice();
      auto const & mm = m_objectManager->GetMemoryManager();

      m_dedicatedStagingBuffer = m_objectManager->CreateBuffer(kStagingBuffer, bufferSize, 0 /* batcherHash */);
      VkMemoryRequirements memReqs = {};
      vkGetBufferMemoryRequirements(device, m_dedicatedStagingBuffer.m_buffer, &memReqs);

      // We must be able to map the whole range.
      size_t const sizeAlignment = mm.GetSizeAlignment(memReqs);
      auto const alignedSize = mm.GetAligned(bufferSize, sizeAlignment);
      if (bufferSize > alignedSize)
      {
        // This GPU uses non-standard alignment we have to recreate buffer.
        bufferSize = VulkanMemoryManager::GetAligned(bufferSize, sizeAlignment);
        m_objectManager->DestroyObjectUnsafe(m_dedicatedStagingBuffer);
        m_dedicatedStagingBuffer = m_objectManager->CreateBuffer(kStagingBuffer, bufferSize, 0 /* batcherHash */);
        vkGetBufferMemoryRequirements(device, m_dedicatedStagingBuffer.m_buffer, &memReqs);
      }

      if (data != nullptr)
        m_objectManager->Fill(m_dedicatedStagingBuffer, data.get(), notAlignedSize);

      m_copyRegion = BufferCopyRegion(0, 0, params.m_width, params.m_height, 0);
    }
    m_usePersistentStagingBuffer = params.m_usePersistentStagingBuffer;

    // Create image.
    m_textureObject = m_objectManager->CreateImage(VK_IMAGE_USAGE_SAMPLED_BIT, format, tiling,
                                                   VK_IMAGE_ASPECT_COLOR_BIT, params.m_width, params.m_height);
  }
}

void VulkanTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width,
                               uint32_t height, ref_ptr<void> data)
{
  CHECK(m_isMutable, ("Upload data is avaivable only for mutable textures."));
  CHECK(m_objectManager != nullptr, ());
  CHECK(data != nullptr, ());

  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkCommandBuffer commandBuffer = vulkanContext->GetCurrentMemoryCommandBuffer();
  if (commandBuffer == nullptr)
  {
    // Upload may happen in backend renderer.
    // Copy data to staging buffer that will be used in frontend renderer in the closest Bind() call.
    std::lock_guard<std::mutex> lock(m_dedicatedStagingBufferMutex);
    CHECK(m_dedicatedStagingBuffer.m_buffer, ());
    auto const bufferSize = GetBytesPerPixel(GetFormat()) * width * height;
    CHECK(bufferSize <= m_dedicatedStagingBuffer.GetAlignedSize(), ());
    m_objectManager->Fill(m_dedicatedStagingBuffer, data.get(), bufferSize);
    m_copyRegion = BufferCopyRegion(x, y, width, height, 0);
    return;
  }

  Bind(context);

  auto const sizeInBytes = GetBytesPerPixel(GetFormat()) * width * height;

  VkBuffer sb;
  uint32_t offset;
  auto stagingBuffer = vulkanContext->GetDefaultStagingBuffer();
  if (stagingBuffer->HasEnoughSpace(sizeInBytes))
  {
    auto staging = stagingBuffer->Reserve(sizeInBytes);
    memcpy(staging.m_pointer, data.get(), sizeInBytes);
    sb = staging.m_stagingBuffer;
    offset = staging.m_offset;
  }
  else
  {
    // Here we use temporary staging object, which will be destroyed after the nearest
    // command queue submitting.
    VulkanStagingBuffer tempStagingBuffer(m_objectManager, sizeInBytes);
    CHECK(tempStagingBuffer.HasEnoughSpace(sizeInBytes), ());
    auto staging = tempStagingBuffer.Reserve(sizeInBytes);
    memcpy(staging.m_pointer, data.get(), sizeInBytes);
    tempStagingBuffer.Flush();
    sb = staging.m_stagingBuffer;
    offset = staging.m_offset;
  }

  // Here we use VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, because we also read textures
  // in vertex shaders.
  MakeImageLayoutTransition(
      commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT);

  auto bufferCopyRegion = BufferCopyRegion(x, y, width, height, offset);
  vkCmdCopyBufferToImage(commandBuffer, sb, m_textureObject.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferCopyRegion);

  MakeImageLayoutTransition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void VulkanTexture::Bind(ref_ptr<dp::GraphicsContext> context) const
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkCommandBuffer commandBuffer = vulkanContext->GetCurrentMemoryCommandBuffer();
  CHECK(commandBuffer != nullptr, ());

  // Fill texture on the bind if dedicated staging buffer is present.
  {
    std::lock_guard<std::mutex> lock(m_dedicatedStagingBufferMutex);
    if (m_dedicatedStagingBuffer.m_buffer)
    {
      // Here we use VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, because we also read textures
      // in vertex shaders.
      MakeImageLayoutTransition(
          commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT);

      vkCmdCopyBufferToImage(commandBuffer, m_dedicatedStagingBuffer.m_buffer, m_textureObject.m_image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &m_copyRegion);

      MakeImageLayoutTransition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

      // Release dedicated staging buffer if it's not persistent.
      if (!m_usePersistentStagingBuffer)
      {
        m_objectManager->DestroyObject(m_dedicatedStagingBuffer);
        m_dedicatedStagingBuffer = {};
      }
    }
  }
}

void VulkanTexture::SetFilter(TextureFilter filter)
{
  m_params.m_filter = filter;
}

bool VulkanTexture::Validate() const
{
  return m_textureObject.m_image != VK_NULL_HANDLE && m_textureObject.m_imageView != VK_NULL_HANDLE;
}

SamplerKey VulkanTexture::GetSamplerKey() const
{
  return SamplerKey(m_params.m_filter, m_params.m_wrapSMode, m_params.m_wrapTMode);
}

void VulkanTexture::MakeImageLayoutTransition(VkCommandBuffer commandBuffer, VkImageLayout newLayout,
                                              VkPipelineStageFlags srcStageMask,
                                              VkPipelineStageFlags dstStageMask) const
{
  VkAccessFlags srcAccessMask = 0;
  VkAccessFlags dstAccessMask = 0;

  VkPipelineStageFlags const noAccessMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
                                            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  VkPipelineStageFlags srcRemainingMask = srcStageMask & ~noAccessMask;
  VkPipelineStageFlags dstRemainingMask = dstStageMask & ~noAccessMask;

  auto const srcTestAndRemoveBit = [&](VkPipelineStageFlagBits stageBit, VkAccessFlags accessBits)
  {
    if (srcStageMask & stageBit)
    {
      srcAccessMask |= accessBits;
      srcRemainingMask &= ~stageBit;
    }
  };

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

  srcTestAndRemoveBit(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  CHECK(srcRemainingMask == 0, ("Not implemented transition for src pipeline stage"));

  auto const dstTestAndRemoveBit = [&](VkPipelineStageFlagBits stageBit, VkAccessFlags accessBits)
  {
    if (dstStageMask & stageBit)
    {
      dstAccessMask |= accessBits;
      dstRemainingMask &= ~stageBit;
    }
  };

  dstTestAndRemoveBit(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  dstTestAndRemoveBit(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

  dstTestAndRemoveBit(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

  dstTestAndRemoveBit(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);

  dstTestAndRemoveBit(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

  CHECK(dstRemainingMask == 0, ("Not implemented transition for dest pipeline stage"));

  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = nullptr;
  imageMemoryBarrier.srcAccessMask = srcAccessMask;
  imageMemoryBarrier.dstAccessMask = dstAccessMask;
  imageMemoryBarrier.oldLayout = m_currentLayout;
  imageMemoryBarrier.newLayout = newLayout;
  imageMemoryBarrier.image = GetImage();
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.subresourceRange.aspectMask = m_aspectFlags;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

  m_currentLayout = newLayout;
}
}  // namespace vulkan
}  // namespace dp
