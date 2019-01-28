#include "drape/vulkan/vulkan_base_context.hpp"

#include "drape/framebuffer.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace dp
{
namespace vulkan
{
VulkanBaseContext::VulkanBaseContext(VkInstance vulkanInstance, VkPhysicalDevice gpu,
                                     VkPhysicalDeviceProperties const & gpuProperties,
                                     VkDevice device, uint32_t renderingQueueFamilyIndex,
                                     ref_ptr<VulkanObjectManager> objectManager)
  : m_vulkanInstance(vulkanInstance)
  , m_gpu(gpu)
  , m_gpuProperties(gpuProperties)
  , m_device(device)
  , m_renderingQueueFamilyIndex(renderingQueueFamilyIndex)
  , m_objectManager(objectManager)
{}

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

void VulkanBaseContext::Present()
{
  // Resetting of the default staging buffer must be before submitting the queue.
  // It guarantees the graphics data coherence.
  m_objectManager->FlushDefaultStagingBuffer();

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PrePresent)])
    h.second(make_ref(this));

  // TODO: wait for all map-memory operations.

  // TODO: submit queue, wait for finishing of rendering.

  for (auto const & h : m_handlers[static_cast<uint32_t>(HandlerType::PostPresent)])
    h.second(make_ref(this));

  // Resetting of the default staging buffer and collecting destroyed objects must be
  // only after the finishing of rendering. It prevents data collisions.
  m_objectManager->ResetDefaultStagingBuffer();
  m_objectManager->CollectObjects();
}

uint32_t VulkanBaseContext::RegisterHandler(HandlerType handlerType, ContextHandler && handler)
{
  static uint32_t counter = 0;
  CHECK_LESS(counter, std::numeric_limits<uint32_t>::max(), ());
  ASSERT(handler != nullptr, ());
  uint32_t const id = ++counter;
  m_handlers[static_cast<uint32_t>(handlerType)].emplace_back(std::make_pair(id, std::move(handler)));
  return id;
}

void VulkanBaseContext::UnregisterHandler(uint32_t id)
{
  for (size_t i = 0; i < m_handlers.size(); ++i)
  {
    m_handlers[i].erase(std::remove_if(m_handlers[i].begin(), m_handlers[i].end(),
                                       [id](std::pair<uint8_t, ContextHandler> const & p)
    {
      return p.first == id;
    }), m_handlers[i].end());
  }
}
}  // namespace vulkan
}  // namespace dp
