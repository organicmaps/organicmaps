#include "drape/vulkan/vulkan_utils.hpp"

#include <array>

namespace dp
{
namespace vulkan
{
namespace
{
// Sampler package.
uint8_t constexpr kWrapSModeByte = 3;
uint8_t constexpr kWrapTModeByte = 2;
uint8_t constexpr kMagFilterByte = 1;
uint8_t constexpr kMinFilterByte = 0;
}  // namespace

std::string GetVulkanResultString(VkResult result)
{
  switch (result)
  {
  case VK_SUCCESS: return "VK_SUCCESS";
  case VK_NOT_READY: return "VK_NOT_READY";
  case VK_TIMEOUT: return "VK_TIMEOUT";
  case VK_EVENT_SET: return "VK_EVENT_SET";
  case VK_EVENT_RESET: return "VK_EVENT_RESET";
  case VK_INCOMPLETE: return "VK_INCOMPLETE";
  case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
  case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
  case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
  case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
  case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
  case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
  case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
  case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
  case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
  case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
  case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
  case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
  case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
    return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
  case VK_ERROR_FRAGMENTATION_EXT: return "VK_ERROR_FRAGMENTATION_EXT";
  case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
  case VK_ERROR_OUT_OF_POOL_MEMORY_KHR: return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR: return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR";
  case VK_RESULT_RANGE_SIZE: return "VK_RESULT_RANGE_SIZE";
  case VK_RESULT_MAX_ENUM: return "VK_RESULT_MAX_ENUM";
  }
  UNREACHABLE();
  return "Unknown result";
}

VkFormat UnpackFormat(TextureFormat format)
{
  switch (format)
  {
  case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
  case TextureFormat::Alpha: return VK_FORMAT_R8_UNORM;
  case TextureFormat::RedGreen: return VK_FORMAT_R8G8_UNORM;
  case TextureFormat::DepthStencil: return VK_FORMAT_D24_UNORM_S8_UINT;
  case TextureFormat::Depth: return VK_FORMAT_D16_UNORM;
  case TextureFormat::Unspecified:
    CHECK(false, ());
    return VK_FORMAT_UNDEFINED;
  }
  CHECK(false, ());
}

SamplerKey::SamplerKey(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode)
{
  Set(filter, wrapSMode, wrapTMode);
}

void SamplerKey::Set(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode)
{
  SetStateByte(m_sampler, static_cast<uint8_t>(filter), kMinFilterByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(filter), kMagFilterByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(wrapSMode), kWrapSModeByte);
  SetStateByte(m_sampler, static_cast<uint8_t>(wrapTMode), kWrapTModeByte);
}

TextureFilter SamplerKey::GetTextureFilter() const
{
  return static_cast<TextureFilter>(GetStateByte(m_sampler, kMinFilterByte));
}

TextureWrapping SamplerKey::GetWrapSMode() const
{
  return static_cast<TextureWrapping>(GetStateByte(m_sampler, kWrapSModeByte));
}

TextureWrapping SamplerKey::GetWrapTMode() const
{
  return static_cast<TextureWrapping>(GetStateByte(m_sampler, kWrapTModeByte));
}

bool SamplerKey::operator<(SamplerKey const & rhs) const
{
  return m_sampler < rhs.m_sampler;
}

void DescriptorSetGroup::Update(VkDevice device, std::vector<ParamDescriptor> const & descriptors)
{
  std::vector<VkWriteDescriptorSet> writeDescriptorSets(descriptors.size());
  for (size_t i = 0; i < writeDescriptorSets.size(); ++i)
  {
    writeDescriptorSets[i] = {};
    writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[i].dstSet = m_descriptorSet;
    writeDescriptorSets[i].descriptorCount = 1;
    if (descriptors[i].m_type == ParamDescriptor::Type::DynamicUniformBuffer)
    {
      writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
      writeDescriptorSets[i].dstBinding = 0;
      writeDescriptorSets[i].pBufferInfo = &descriptors[i].m_bufferDescriptor;
    }
    else if (descriptors[i].m_type == ParamDescriptor::Type::Texture)
    {
      writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(descriptors[i].m_textureSlot);
      writeDescriptorSets[i].pImageInfo = &descriptors[i].m_imageDescriptor;
    }
    else
    {
      CHECK(false, ("Unsupported param descriptor type."));
    }
  }

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, nullptr);
}
}  // namespace vulkan
}  // namespace dp
