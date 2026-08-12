#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"

#include <utility>

namespace vulkan_context_tests
{
using dp::vulkan::VulkanBaseContext;
using HandlerType = VulkanBaseContext::HandlerType;

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

  void AddHandler(HandlerType type, uint32_t id, ContextHandler && handler)
  {
    m_handlers[static_cast<size_t>(type)].emplace_back(id, std::move(handler));
  }

  void InvokeHandlers()
  {
    for (auto const & handlers : m_handlers)
      for (auto const & handler : handlers)
        handler.second(0 /* inflightFrameIndex */);
  }
};

UNIT_TEST(VulkanContext_UnregisterHighHandlerIds)
{
  IdleMock idleMock;
  uint32_t calls = 0;
  Context context;
  uint32_t constexpr kHandlersPerType = 256;

  for (auto const type : {HandlerType::PrePresent, HandlerType::PostPresent, HandlerType::UpdateInflightFrame})
  {
    uint32_t id = 0;
    for (uint32_t i = 0; i < kHandlersPerType; ++i)
      id = context.RegisterHandler(type, [&calls](uint32_t) { ++calls; });

    TEST_GREATER_OR_EQUAL(id, kHandlersPerType, ());
    context.UnregisterHandler(id);
  }

  context.InvokeHandlers();
  TEST_EQUAL(calls, 3 * (kHandlersPerType - 1), ());
}

UNIT_TEST(VulkanContext_UnregisterPreservesIdsWithMatchingLowByte)
{
  IdleMock idleMock;
  uint32_t removedCalls = 0;
  uint32_t retainedCalls = 0;
  Context context;

  // Explicit IDs make the collision independent of registrations in other tests.
  context.AddHandler(HandlerType::PostPresent, 1, [&removedCalls](uint32_t) { ++removedCalls; });
  context.AddHandler(HandlerType::PostPresent, 257, [&retainedCalls](uint32_t) { ++retainedCalls; });
  context.UnregisterHandler(1);

  context.InvokeHandlers();
  TEST_EQUAL(removedCalls, 0, ());
  TEST_EQUAL(retainedCalls, 1, ());
}
}  // namespace vulkan_context_tests
