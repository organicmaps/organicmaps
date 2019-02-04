#include "drape/vulkan/vulkan_texture.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_staging_buffer.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "base/logging.hpp"

drape_ptr<dp::HWTextureAllocator> CreateVulkanAllocator()
{
  return make_unique_dp<dp::vulkan::VulkanTextureAllocator>();
}

ref_ptr<dp::HWTextureAllocator> GetDefaultVulkanAllocator()
{
  static dp::vulkan::VulkanTextureAllocator allocator;
  return make_ref<dp::HWTextureAllocator>(&allocator);
}

namespace dp
{
namespace vulkan
{
namespace
{
VkFormat UnpackFormat(TextureFormat format)
{
  switch (format)
  {
  case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
  case TextureFormat::Alpha: return VK_FORMAT_R8_UNORM;
  case TextureFormat::RedGreen: return VK_FORMAT_R8G8_UNORM;
  case TextureFormat::DepthStencil: return VK_FORMAT_D24_UNORM_S8_UINT;
  case TextureFormat::Depth: return VK_FORMAT_D32_SFLOAT;
  case TextureFormat::Unspecified:
    CHECK(false, ());
    return VK_FORMAT_UNDEFINED;
  }
  CHECK(false, ());
}

VkImageMemoryBarrier PreTransferBarrier(VkImageLayout initialLayout, VkImage image)
{
  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = nullptr;
  imageMemoryBarrier.srcAccessMask = (initialLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ?
                                     VK_ACCESS_SHADER_READ_BIT : 0;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.oldLayout = initialLayout;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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

VkImageMemoryBarrier PostTransferBarrier(VkImage image)
{
  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.pNext = nullptr;
  imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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

VkBufferImageCopy BufferCopyRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                   uint32_t stagingOffset)
{
  VkBufferImageCopy bufferCopyRegion = {};
  bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferCopyRegion.imageSubresource.mipLevel = 1;
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
  return make_unique_dp<VulkanTexture>(make_ref<VulkanTextureAllocator>(this));
}

VulkanTexture::VulkanTexture(ref_ptr<VulkanTextureAllocator> allocator)
  : m_allocator(allocator)
{}

VulkanTexture::~VulkanTexture()
{
  m_objectManager->DestroyObject(m_textureObject);
}

void VulkanTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  Base::Create(context, params, data);

  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  m_objectManager = vulkanContext->GetObjectManager();

  auto const format = UnpackFormat(params.m_format);
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(vulkanContext->GetPhysicalDevice(), format, &formatProperties);
  CHECK(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, ());

  m_isMutable = params.m_isMutable;
  if (params.m_isRenderTarget)
  {
    CHECK(false, ());
    //TODO
  }
  else
  {
    auto const bufferSize = GetBytesPerPixel(params.m_format) * params.m_width * params.m_height;

    // Create temporary staging buffer.
    m_creationStagingBuffer = make_unique_dp<VulkanStagingBuffer>(m_objectManager, bufferSize);
    ASSERT(m_creationStagingBuffer->HasEnoughSpace(bufferSize), ());
    VulkanStagingBuffer::StagingData staging;
    m_reservationId = m_creationStagingBuffer->ReserveWithId(bufferSize, staging);
    if (data != nullptr)
      memcpy(staging.m_pointer, data.get(), bufferSize);
    else
      memset(staging.m_pointer, 0, bufferSize);
    m_creationStagingBuffer->Flush();

    // Create image.
    m_textureObject = m_objectManager->CreateImage(VK_IMAGE_USAGE_SAMPLED_BIT, format,
                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                   params.m_width, params.m_height);
    CHECK_VK_CALL(vkBindImageMemory(vulkanContext->GetDevice(), m_textureObject.m_image,
                                    m_textureObject.GetMemory(), m_textureObject.GetAlignedOffset()));
  }
}

void VulkanTexture::UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y,
                               uint32_t width, uint32_t height, ref_ptr<void> data)
{
  CHECK(m_isMutable, ("Upload data is avaivable only for mutable textures."));
  CHECK(m_creationStagingBuffer == nullptr, ());
  CHECK(m_objectManager != nullptr, ());
  CHECK(data != nullptr, ());

  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkCommandBuffer commandBuffer = vulkanContext->GetCurrentCommandBuffer();
  CHECK(commandBuffer != nullptr, ());

  auto const sizeInBytes = GetBytesPerPixel(GetFormat()) * width * height;

  VkBuffer sb;
  uint32_t offset;
  auto stagingBuffer = m_objectManager->GetDefaultStagingBuffer();
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

  auto imageMemoryBarrier = PreTransferBarrier(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                               m_textureObject.m_image);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &imageMemoryBarrier);

  auto bufferCopyRegion = BufferCopyRegion(x, y, width, height, offset);
  vkCmdCopyBufferToImage(commandBuffer, sb, m_textureObject.m_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

  // Here we use VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, because we also read textures
  // in vertex shaders.
  imageMemoryBarrier = PostTransferBarrier(m_textureObject.m_image);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &imageMemoryBarrier);
}

void VulkanTexture::Bind(ref_ptr<dp::GraphicsContext> context) const
{
  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  VkCommandBuffer commandBuffer = vulkanContext->GetCurrentCommandBuffer();
  CHECK(commandBuffer != nullptr, ());

  // Fill texture on the first bind.
  if (m_creationStagingBuffer != nullptr)
  {
    auto imageMemoryBarrier = PreTransferBarrier(VK_IMAGE_LAYOUT_UNDEFINED, m_textureObject.m_image);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageMemoryBarrier);

    auto staging = m_creationStagingBuffer->GetReservationById(m_reservationId);
    auto bufferCopyRegion = BufferCopyRegion(0, 0, GetWidth(), GetHeight(), staging.m_offset);
    vkCmdCopyBufferToImage(commandBuffer, staging.m_stagingBuffer, m_textureObject.m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

    // Here we use VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, because we also read textures
    // in vertex shaders.
    imageMemoryBarrier = PostTransferBarrier(m_textureObject.m_image);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageMemoryBarrier);

    m_creationStagingBuffer.reset();
  }
}

void VulkanTexture::SetFilter(TextureFilter filter)
{
  m_params.m_filter = filter;
}

bool VulkanTexture::Validate() const
{
  return m_textureObject.m_image != 0 && m_textureObject.m_imageView != 0;
}
}  // namespace vulkan
}  // namespace dp
