#pragma once

#include "drape/texture_types.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <vulkan_wrapper.h>

#include <string>
#include <vector>

namespace dp
{
namespace vulkan
{
extern std::string GetVulkanResultString(VkResult result);

uint32_t constexpr kMaxInflightFrames = 2;

class VulkanFormatUnpacker
{
public:
  static bool Init(VkPhysicalDevice gpu);
  static VkFormat Unpack(TextureFormat format);

private:
  static VkFormat m_bestDepthFormat;
};

template<typename T>
void SetStateByte(T & state, uint8_t value, uint8_t byteNumber)
{
  auto const shift = byteNumber * 8;
  auto const mask = ~(static_cast<T>(0xFF) << shift);
  state = (state & mask) | (static_cast<T>(value) << shift);
}

template<typename T>
uint8_t GetStateByte(T & state, uint8_t byteNumber)
{
  return static_cast<uint8_t>((state >> byteNumber * 8) & 0xFF);
}

struct SamplerKey
{
  SamplerKey() = default;
  SamplerKey(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode);
  void Set(TextureFilter filter, TextureWrapping wrapSMode, TextureWrapping wrapTMode);
  TextureFilter GetTextureFilter() const;
  TextureWrapping GetWrapSMode() const;
  TextureWrapping GetWrapTMode() const;
  bool operator<(SamplerKey const & rhs) const;

  uint32_t m_sampler = 0;
};
}  // namespace vulkan
}  // namespace dp

#define LOG_ERROR_VK_CALL(method, statusCode) \
  LOG(LDEBUG, ("Vulkan error:", #method, "finished with code", \
               dp::vulkan::GetVulkanResultString(statusCode)));

#define LOG_ERROR_VK(message) LOG(LDEBUG, ("Vulkan error:", message));

#define CHECK_VK_CALL(method) \
  do { \
    VkResult const statusCode = method; \
    CHECK(statusCode == VK_SUCCESS, ("Vulkan error:", #method, "finished with code", \
                                     dp::vulkan::GetVulkanResultString(statusCode))); \
  } while (false)

#define CHECK_RESULT_VK_CALL(method, statusCode) \
  do { \
    CHECK(statusCode == VK_SUCCESS, ("Vulkan error:", #method, "finished with code", \
                                     dp::vulkan::GetVulkanResultString(statusCode))); \
  } while (false)
