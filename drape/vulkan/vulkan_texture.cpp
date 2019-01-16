#include "drape/vulkan/vulkan_texture.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"

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

}

void VulkanTexture::Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data)
{
  Base::Create(context, params, data);

  ref_ptr<dp::vulkan::VulkanBaseContext> vulkanContext = context;
  m_deviceHolder = vulkanContext->GetDeviceHolder();

  auto devicePtr = m_deviceHolder.lock();
  CHECK(devicePtr != nullptr, ());

  auto const format = UnpackFormat(params.m_format);
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(vulkanContext->GetPhysicalDevice(), format, &formatProperties);
  CHECK(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, ());

  m_isMutable = params.m_isMutable;
  if (params.m_isRenderTarget)
  {
    CHECK(false, ());
//    texDesc.usage |= MTLTextureUsageRenderTarget;
//    texDesc.storageMode = MTLStorageModePrivate;
//    m_texture = [metalDevice newTextureWithDescriptor:texDesc];
//    CHECK(m_texture != nil, ());
  }
  else
  {
//    texDesc.storageMode = MTLStorageModeShared;
//    m_texture = [metalDevice newTextureWithDescriptor:texDesc];
//    CHECK(m_texture != nil, ());
//    MTLRegion region = MTLRegionMake2D(0, 0, m_params.m_width, m_params.m_height);
//    auto const rowBytes = m_params.m_width * GetBytesPerPixel(m_params.m_format);
//    [m_texture replaceRegion:region mipmapLevel:0 withBytes:data.get() bytesPerRow:rowBytes];
  }
}

void VulkanTexture::UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data)
{
  CHECK(m_isMutable, ("Upload data is avaivable only for mutable textures."));
  CHECK(false, ());
//  MTLRegion region = MTLRegionMake2D(x, y, width, height);
//  auto const rowBytes = width * GetBytesPerPixel(m_params.m_format);
//  [m_texture replaceRegion:region mipmapLevel:0 withBytes:data.get() bytesPerRow:rowBytes];
}

void VulkanTexture::SetFilter(TextureFilter filter)
{
  m_params.m_filter = filter;
}

bool VulkanTexture::Validate() const
{
  return m_texture != nullptr && m_textureView != nullptr;
}
}  // namespace vulkan
}  // namespace dp
