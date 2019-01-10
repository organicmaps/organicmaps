#include "drape/vulkan/vulkan_base_context.hpp"

#include "drape/framebuffer.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace dp
{
namespace vulkan
{
VulkanBaseContext::VulkanBaseContext(VkInstance vulkanInstance, VkPhysicalDevice gpu,
                                     VkDevice device)
  : m_vulkanInstance(vulkanInstance)
  , m_gpu(gpu)
  , m_device(device)
{
  vkGetPhysicalDeviceProperties(m_gpu, &m_gpuProperties);
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

void VulkanBaseContext::SetStencilReferenceValue(uint32_t stencilReferenceValue)
{
  m_stencilReferenceValue = stencilReferenceValue;
}

void VulkanBaseContext::SetSurface(VkSurfaceKHR surface, VkFormat surfaceFormat,
                                   int width, int height)
{
  m_surface = surface;
  //TODO: initialize swapchains, image views and so on.
}

void VulkanBaseContext::ResetSurface()
{
  vkDeviceWaitIdle(m_device);

  //TODO: reset swapchains, image views and so on.
  m_surface.reset();
}
}  // namespace vulkan
}  // namespace dp
