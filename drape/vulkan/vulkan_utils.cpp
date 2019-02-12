#include "drape/vulkan/vulkan_utils.hpp"

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

VkSamplerAddressMode GetVulkanSamplerAddressMode(TextureWrapping wrapping)
{
  switch (wrapping)
  {
  case TextureWrapping::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case TextureWrapping::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }
  UNREACHABLE();
}

VkFilter GetVulkanFilter(TextureFilter filter)
{
  switch (filter)
  {
  case TextureFilter::Linear: return VK_FILTER_LINEAR;
  case TextureFilter::Nearest: return VK_FILTER_NEAREST;
  }
  UNREACHABLE();
}
}  // namespace vulkan
}  // namespace dp
