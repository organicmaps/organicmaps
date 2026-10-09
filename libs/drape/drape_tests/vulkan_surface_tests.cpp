#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include <limits>

namespace vulkan_surface_tests
{
using dp::vulkan::ChooseCompositeAlpha;
using dp::vulkan::ChooseSurfaceExtent;

VkSurfaceCapabilitiesKHR Capabilities()
{
  VkSurfaceCapabilitiesKHR capabilities = {};
  capabilities.currentExtent = {640, 480};
  capabilities.minImageExtent = {100, 100};
  capabilities.maxImageExtent = {1024, 1024};
  return capabilities;
}

void CheckExtent(VkSurfaceCapabilitiesKHR const & capabilities, VkExtent2D framebufferSize, VkExtent2D expected)
{
  auto const extent = ChooseSurfaceExtent(capabilities, framebufferSize);
  TEST_EQUAL(extent.width, expected.width, ());
  TEST_EQUAL(extent.height, expected.height, ());
}

UNIT_TEST(VulkanSurface_FixedExtent)
{
  CheckExtent(Capabilities(), {800, 600}, {640, 480});
}

UNIT_TEST(VulkanSurface_UnspecifiedExtent)
{
  auto capabilities = Capabilities();
  auto const unspecified = std::numeric_limits<uint32_t>::max();
  capabilities.currentExtent = {unspecified, unspecified};
  CheckExtent(capabilities, {800, 600}, {800, 600});
  CheckExtent(capabilities, {50, 2000}, {100, 1024});
}

UNIT_TEST(VulkanSurface_ZeroExtentDefersAllocation)
{
  auto capabilities = Capabilities();
  CheckExtent(capabilities, {0, 480}, {});
  CheckExtent(capabilities, {640, 0}, {});
  capabilities.currentExtent = {};
  CheckExtent(capabilities, {640, 480}, {});
  auto const unspecified = std::numeric_limits<uint32_t>::max();
  capabilities.currentExtent = {unspecified, unspecified};
  capabilities.minImageExtent = capabilities.maxImageExtent = {};
  CheckExtent(capabilities, {640, 480}, {});
}

VkSurfaceCapabilitiesKHR g_capabilities;
VkResult g_capabilitiesResult;
unsigned g_queries;
VkResult g_fenceResult;
uint64_t g_fenceTimeout;
dp::GraphicsContext * g_pauseDuringFenceWait;

VKAPI_ATTR VkResult VKAPI_CALL FakeDeviceWaitIdle(VkDevice)
{
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL FakeWaitForFences(VkDevice, uint32_t, VkFence const *, VkBool32, uint64_t timeout)
{
  g_fenceTimeout = timeout;
  if (g_pauseDuringFenceWait)
    g_pauseDuringFenceWait->SetPresentAvailable(false);
  return g_fenceResult;
}

VKAPI_ATTR VkResult VKAPI_CALL FakeGetSurfaceCapabilities(VkPhysicalDevice, VkSurfaceKHR,
                                                          VkSurfaceCapabilitiesKHR * capabilities)
{
  ++g_queries;
  *capabilities = g_capabilities;
  return g_capabilitiesResult;
}

class VulkanCallsGuard
{
public:
  VulkanCallsGuard()
  {
    vkDeviceWaitIdle = &FakeDeviceWaitIdle;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = &FakeGetSurfaceCapabilities;
    vkWaitForFences = &FakeWaitForFences;
    g_queries = 0;
    g_capabilities = Capabilities();
    g_capabilitiesResult = VK_SUCCESS;
    g_fenceResult = VK_TIMEOUT;
    g_pauseDuringFenceWait = nullptr;
  }
  ~VulkanCallsGuard()
  {
    vkDeviceWaitIdle = m_waitIdle;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = m_getCapabilities;
    vkWaitForFences = m_waitForFences;
  }

private:
  PFN_vkDeviceWaitIdle m_waitIdle = vkDeviceWaitIdle;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR m_getCapabilities = vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
  PFN_vkWaitForFences m_waitForFences = vkWaitForFences;
};

class TestContext : public dp::vulkan::VulkanBaseContext
{
public:
  void MakeCurrent() override {}

  TestContext()
    : VulkanBaseContext(VK_NULL_HANDLE, VK_NULL_HANDLE, {}, VK_NULL_HANDLE, 0, nullptr, nullptr, false, true)
  {
    // A sentinel surface is enough: mocked entry points never use the handle.
    m_surface = VkSurfaceKHR{};
    m_framebufferSize = {640, 480};
  }

  ~TestContext() override { m_swapchain = VK_NULL_HANDLE; }

  void UseSwapchain()
  {
#if VK_USE_64_BIT_PTR_DEFINES
    m_swapchain = reinterpret_cast<VkSwapchainKHR>(1);
#else
    m_swapchain = 1;
#endif
  }
};

UNIT_TEST(VulkanSurface_ContextSuspendsBeforeAllocatingAtZeroExtent)
{
  VulkanCallsGuard calls;
  TestContext context;
  g_capabilities.currentExtent = {};
  TEST(context.BeginRendering() == dp::FrameStatus::Suspended, ());
  TEST_EQUAL(g_queries, 1, ());
}

UNIT_TEST(VulkanSurface_UnavailablePresentationDoesNotQueryOrAllocate)
{
  VulkanCallsGuard calls;
  TestContext context;
  context.SetPresentAvailable(false);
  TEST(context.BeginRendering() == dp::FrameStatus::Suspended, ());
  TEST_EQUAL(g_queries, 0, ());
}

UNIT_TEST(VulkanSurface_ZeroResizeDefersAllocation)
{
  VulkanCallsGuard calls;
  TestContext context;
  context.Resize(0, 480);
  TEST(context.BeginRendering() == dp::FrameStatus::Suspended, ());
  TEST_EQUAL(g_queries, 2, ());
}

UNIT_TEST(VulkanSurface_LostSurfaceRetainsTransientRetry)
{
  VulkanCallsGuard calls;
  TestContext context;
  g_capabilitiesResult = VK_ERROR_SURFACE_LOST_KHR;
  TEST(context.BeginRendering() == dp::FrameStatus::RetryLater, ());
}

UNIT_TEST(VulkanSurface_FenceTimeoutRemainsRetryable)
{
  VulkanCallsGuard calls;
  TestContext context;
  context.UseSwapchain();
  TEST(context.BeginRendering() == dp::FrameStatus::RetryLater, ());
  TEST(g_fenceTimeout > 0 && g_fenceTimeout < std::numeric_limits<uint64_t>::max(), ());
}

UNIT_TEST(VulkanSurface_PauseDuringFenceWaitSkipsAcquisition)
{
  VulkanCallsGuard calls;
  TestContext context;
  context.UseSwapchain();
  g_fenceResult = VK_SUCCESS;
  g_pauseDuringFenceWait = &context;
  TEST(context.BeginRendering() == dp::FrameStatus::Suspended, ());
}
UNIT_TEST(VulkanSurface_CompositeAlpha)
{
  VkCompositeAlphaFlagBitsKHR const modes[] = {VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                               VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                               VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR};
  for (auto const mode : modes)
    TEST(ChooseCompositeAlpha(mode) == mode, ());

  TEST(ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ==
           VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
       ());
  TEST(ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR | VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ==
           VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
       ());
  TEST(ChooseCompositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ==
           VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
       ());
  TEST(!ChooseCompositeAlpha(0), ());
}
}  // namespace vulkan_surface_tests
