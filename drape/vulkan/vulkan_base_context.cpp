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
  ss << "API:" << m_gpuProperties.apiVersion << "/Driver:" << m_gpuProperties.driverVersion;
  return ss.str();
}

void VulkanBaseContext::SetStencilReferenceValue(uint32_t stencilReferenceValue)
{
  m_stencilReferenceValue = stencilReferenceValue;
}

void VulkanBaseContext::SetSurface(VkSurfaceKHR surface)
{
  m_surface = surface;
}

void VulkanBaseContext::ResetSurface()
{
  m_surface.reset();
}
}  // namespace vulkan
}  // namespace dp
