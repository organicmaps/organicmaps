#include "std/target_os.hpp"
#if !defined(OMIM_OS_WINDOWS)
#error Unsupported OS
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan_wrapper.h>

#include "drape/vulkan/vulkan_context_factory.hpp"

#include <array>
#include <atomic>
#include <memory>

class WindowsVulkanContextFactory : public dp::vulkan::VulkanContextFactory
{
public:
  WindowsVulkanContextFactory() : dp::vulkan::VulkanContextFactory(1, 33, false) {}

  void SetSurface(HWND hwnd)
  {
    VkWin32SurfaceCreateInfoKHR const createInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .hinstance = GetModuleHandleW(nullptr),
        .hwnd = hwnd,
    };

    VkResult statusCode;
    CHECK(vkCreateWin32SurfaceKHR, ());
    statusCode = vkCreateWin32SurfaceKHR(m_vulkanInstance, &createInfo, nullptr, &m_surface);
    if (statusCode != VK_SUCCESS)
    {
      LOG_ERROR_VK_CALL(vkCreateWin32SurfaceKHR, statusCode);
      return;
    }

    uint32_t const renderingQueueIndex = m_drawContext->GetRenderingQueueFamilyIndex();
    VkBool32 supportsPresent;
    statusCode = vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, renderingQueueIndex, m_surface, &supportsPresent);
    if (statusCode != VK_SUCCESS)
    {
      LOG_ERROR_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, statusCode);
      return;
    }
    CHECK_EQUAL(supportsPresent, VK_TRUE, ());

    CHECK(QuerySurfaceSize(), ());

    if (m_drawContext)
      m_drawContext->SetSurface(m_surface, m_surfaceFormat, m_surfaceCapabilities);
  }

  void ResetSurface()
  {
    if (m_drawContext)
      m_drawContext->ResetSurface(false);

    vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
  }
};

drape_ptr<dp::GraphicsContextFactory> CreateContextFactory(GLFWwindow * window, dp::ApiVersion api, m2::PointU size)
{
  if (api == dp::ApiVersion::Vulkan)
  {
    auto contextFactory = make_unique_dp<WindowsVulkanContextFactory>();
    contextFactory->SetSurface(glfwGetWin32Window(window));
    return contextFactory;
  }

  ASSERT(false, ("API is not available yet"));
  return nullptr;
}

void OnCreateDrapeEngine(GLFWwindow * window, dp::ApiVersion api, ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  // Do nothing
}

void PrepareDestroyContextFactory(ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  auto const api = contextFactory->GetDrawContext()->GetApiVersion();
  if (api == dp::ApiVersion::Vulkan)
  {
    ref_ptr<WindowsVulkanContextFactory> windowsContextFactory = contextFactory;
    windowsContextFactory->ResetSurface();
  }
  else
  {
    ASSERT(false, ("API is not available yet"));
  }
}

void UpdateContentScale(GLFWwindow * window, float scale)
{
  // Do nothing
}

void UpdateSize(ref_ptr<dp::GraphicsContextFactory> contextFactory, int w, int h)
{
  // Do nothing
}
