#pragma once

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <string>

namespace dp
{
namespace vulkan
{
extern std::string GetVulkanResultString(VkResult result);

struct ParamDescriptor
{
  enum class Type : uint8_t
  {
    DynamicUniformBuffer,
    Texture
  };

  Type m_type = Type::DynamicUniformBuffer;

  VkDescriptorBufferInfo m_bufferDescriptor = {};
  uint32_t m_bufferDynamicOffset = 0;

  VkDescriptorImageInfo m_imageDescriptor = {};
  int8_t m_textureSlot = 0;
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
