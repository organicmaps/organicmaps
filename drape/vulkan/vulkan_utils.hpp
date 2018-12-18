#pragma once

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <string>

namespace dp
{
namespace vulkan
{
extern std::string GetVulkanResultString(VkResult result);
}  // namespace vulkan
}  // namespace dp
