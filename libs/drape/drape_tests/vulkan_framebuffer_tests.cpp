#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"

#include "base/scope_guard.hpp"

#include <type_traits>
#include <utility>

namespace vulkan_framebuffer_tests
{
using dp::vulkan::VulkanBaseContext;

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice)
{
  return VK_SUCCESS;
}

// An empty context only needs the idle entry point for destruction; these tests submit no GPU work.
class IdleMock
{
public:
  IdleMock() : m_original(std::exchange(vkDeviceWaitIdle, &DeviceWaitIdle)) {}
  ~IdleMock() { vkDeviceWaitIdle = m_original; }

private:
  PFN_vkDeviceWaitIdle const m_original;
};

class Context : public VulkanBaseContext
{
public:
  Context() : VulkanBaseContext(VK_NULL_HANDLE, VK_NULL_HANDLE, {}, VK_NULL_HANDLE, 0, nullptr, nullptr, false, false)
  {}

  void MakeCurrent() override {}

  template <typename T>
  static T Handle(uintptr_t value)
  {
    if constexpr (std::is_pointer_v<T>)
      return reinterpret_cast<T>(value);
    else
      return static_cast<T>(value);
  }

  void CacheFramebuffer(dp::BaseFramebuffer & framebuffer)
  {
    m_currentFramebuffer = make_ref(&framebuffer);
    auto & data = m_framebuffersData[m_currentFramebuffer];
    data.m_renderPass = Handle<VkRenderPass>(1);
    data.m_framebuffers = {Handle<VkFramebuffer>(2)};
  }

  bool HasFramebufferCache() const { return m_currentFramebuffer != nullptr || !m_framebuffersData.empty(); }
};

uint32_t g_destroyedFramebuffers = 0;
uint32_t g_destroyedRenderPasses = 0;

VKAPI_ATTR void VKAPI_CALL DestroyFramebuffer(VkDevice, VkFramebuffer, VkAllocationCallbacks const *)
{
  ++g_destroyedFramebuffers;
}

VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(VkDevice, VkRenderPass, VkAllocationCallbacks const *)
{
  ++g_destroyedRenderPasses;
}

UNIT_TEST(VulkanContext_ReleaseClearsFramebufferCacheBeforeAddressReuse)
{
  class Framebuffer : public dp::BaseFramebuffer
  {
  public:
    void Bind() override {}
  } framebuffer;

  IdleMock idleMock;
  auto const oldFramebuffer = std::exchange(vkDestroyFramebuffer, &DestroyFramebuffer);
  auto const oldRenderPass = std::exchange(vkDestroyRenderPass, &DestroyRenderPass);
  SCOPE_GUARD(restore, [&]()
  {
    vkDestroyFramebuffer = oldFramebuffer;
    vkDestroyRenderPass = oldRenderPass;
  });
  g_destroyedFramebuffers = g_destroyedRenderPasses = 0;
  {
    Context context;
    for (uint32_t release = 1; release <= 2; ++release)
    {
      context.CacheFramebuffer(framebuffer);
      context.DoneCurrent();
      TEST(!context.HasFramebufferCache(), ("Released textures must not leave cached framebuffer attachments"));
      TEST_EQUAL(g_destroyedFramebuffers, release, ());
      TEST_EQUAL(g_destroyedRenderPasses, release, ());
    }
  }
}
}  // namespace vulkan_framebuffer_tests
