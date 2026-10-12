#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include <limits>
#include <type_traits>
#include <vector>

namespace vulkan_frame_tests
{
template <typename T>
T Handle(uintptr_t value)
{
  if constexpr (std::is_pointer_v<T>)
    return reinterpret_cast<T>(value);
  else
    return static_cast<T>(value);
}

struct Calls
{
  uintptr_t m_nextHandle = 100;
  VkResult m_acquireResult = VK_TIMEOUT;
  uint64_t m_acquireTimeout = 0;
  unsigned m_acquires = 0;
  unsigned m_idleWaits = 0;
  unsigned m_fenceResets = 0;
  unsigned m_commandBegins = 0;
  VkFence m_resetFence = VK_NULL_HANDLE;
  VkSemaphore m_acquireSemaphore = VK_NULL_HANDLE;
  VkSemaphore m_submitWaitSemaphore = VK_NULL_HANDLE;
  VkSemaphore m_submitSignalSemaphore = VK_NULL_HANDLE;
  VkSemaphore m_presentWaitSemaphore = VK_NULL_HANDLE;
  uint32_t m_presentImage = 0;
  std::vector<VkSemaphore> m_createdSemaphores;
  std::vector<VkSemaphore> m_destroyedSemaphores;
} g_calls;

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice)
{
  ++g_calls.m_idleWaits;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(VkDevice, uint32_t, VkFence const *, VkBool32, uint64_t)
{
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImage(VkDevice, VkSwapchainKHR, uint64_t timeout, VkSemaphore semaphore,
                                                VkFence, uint32_t * image)
{
  ++g_calls.m_acquires;
  g_calls.m_acquireTimeout = timeout;
  g_calls.m_acquireSemaphore = semaphore;
  if (g_calls.m_acquireResult == VK_SUCCESS)
    *image = 2;  // Deliberately different from the initial in-flight frame index, zero.
  return g_calls.m_acquireResult;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice, uint32_t count, VkFence const * fences)
{
  TEST_EQUAL(count, 1, ());
  ++g_calls.m_fenceResets;
  g_calls.m_resetFence = fences[0];
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer, VkCommandBufferBeginInfo const *)
{
  ++g_calls.m_commandBegins;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer)
{
  return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL EndRenderPass(VkCommandBuffer) {}

VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(VkQueue, uint32_t count, VkSubmitInfo const * info, VkFence)
{
  TEST_EQUAL(count, 1, ());
  TEST_EQUAL(info->waitSemaphoreCount, 1, ());
  TEST_EQUAL(info->signalSemaphoreCount, 1, ());
  g_calls.m_submitWaitSemaphore = info->pWaitSemaphores[0];
  g_calls.m_submitSignalSemaphore = info->pSignalSemaphores[0];
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresent(VkQueue, VkPresentInfoKHR const * info)
{
  TEST_EQUAL(info->waitSemaphoreCount, 1, ());
  TEST_EQUAL(info->swapchainCount, 1, ());
  g_calls.m_presentWaitSemaphore = info->pWaitSemaphores[0];
  g_calls.m_presentImage = info->pImageIndices[0];
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(VkDevice, VkSemaphoreCreateInfo const *, VkAllocationCallbacks const *,
                                               VkSemaphore * semaphore)
{
  *semaphore = Handle<VkSemaphore>(g_calls.m_nextHandle++);
  g_calls.m_createdSemaphores.push_back(*semaphore);
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroySemaphore(VkDevice, VkSemaphore semaphore, VkAllocationCallbacks const *)
{
  g_calls.m_destroyedSemaphores.push_back(semaphore);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(VkDevice, VkBufferCreateInfo const *, VkAllocationCallbacks const *,
                                            VkBuffer * buffer)
{
  *buffer = Handle<VkBuffer>(g_calls.m_nextHandle++);
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(VkDevice, VkBuffer, VkAllocationCallbacks const *) {}

VKAPI_ATTR void VKAPI_CALL GetBufferRequirements(VkDevice, VkBuffer, VkMemoryRequirements * requirements)
{
  *requirements = {.size = 3 * 1024 * 1024, .alignment = 64, .memoryTypeBits = 1};
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice, VkMemoryAllocateInfo const *, VkAllocationCallbacks const *,
                                              VkDeviceMemory * memory)
{
  *memory = Handle<VkDeviceMemory>(g_calls.m_nextHandle++);
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(VkDevice, VkDeviceMemory, VkAllocationCallbacks const *) {}
VKAPI_ATTR VkResult VKAPI_CALL BindBufferMemory(VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
{
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL MapMemory(VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags,
                                         void ** pointer)
{
  // These frames do not upload payloads; the real staging buffers only reset their reservation metadata.
  static uint8_t byte;
  *pointer = &byte;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(VkDevice, VkDeviceMemory) {}

template <typename Function>
class ScopedEntryPoint
{
public:
  ScopedEntryPoint(Function & entryPoint, Function replacement) : m_entryPoint(entryPoint), m_original(entryPoint)
  {
    m_entryPoint = replacement;
  }
  ~ScopedEntryPoint() { m_entryPoint = m_original; }

private:
  Function & m_entryPoint;
  Function const m_original;
};

class VulkanCalls
{
public:
  VulkanCalls() { g_calls = {}; }

private:
  ScopedEntryPoint<PFN_vkDeviceWaitIdle> m_idle{vkDeviceWaitIdle, &DeviceWaitIdle};
  ScopedEntryPoint<PFN_vkWaitForFences> m_wait{vkWaitForFences, &WaitForFences};
  ScopedEntryPoint<PFN_vkAcquireNextImageKHR> m_acquire{vkAcquireNextImageKHR, &AcquireNextImage};
  ScopedEntryPoint<PFN_vkResetFences> m_reset{vkResetFences, &ResetFences};
  ScopedEntryPoint<PFN_vkBeginCommandBuffer> m_begin{vkBeginCommandBuffer, &BeginCommandBuffer};
  ScopedEntryPoint<PFN_vkEndCommandBuffer> m_end{vkEndCommandBuffer, &EndCommandBuffer};
  ScopedEntryPoint<PFN_vkCmdEndRenderPass> m_endPass{vkCmdEndRenderPass, &EndRenderPass};
  ScopedEntryPoint<PFN_vkQueueSubmit> m_submit{vkQueueSubmit, &QueueSubmit};
  ScopedEntryPoint<PFN_vkQueuePresentKHR> m_present{vkQueuePresentKHR, &QueuePresent};
  ScopedEntryPoint<PFN_vkCreateSemaphore> m_createSemaphore{vkCreateSemaphore, &CreateSemaphore};
  ScopedEntryPoint<PFN_vkDestroySemaphore> m_destroySemaphore{vkDestroySemaphore, &DestroySemaphore};
  ScopedEntryPoint<PFN_vkCreateBuffer> m_createBuffer{vkCreateBuffer, &CreateBuffer};
  ScopedEntryPoint<PFN_vkDestroyBuffer> m_destroyBuffer{vkDestroyBuffer, &DestroyBuffer};
  ScopedEntryPoint<PFN_vkGetBufferMemoryRequirements> m_requirements{vkGetBufferMemoryRequirements,
                                                                     &GetBufferRequirements};
  ScopedEntryPoint<PFN_vkAllocateMemory> m_allocate{vkAllocateMemory, &AllocateMemory};
  ScopedEntryPoint<PFN_vkFreeMemory> m_free{vkFreeMemory, &FreeMemory};
  ScopedEntryPoint<PFN_vkBindBufferMemory> m_bind{vkBindBufferMemory, &BindBufferMemory};
  ScopedEntryPoint<PFN_vkMapMemory> m_map{vkMapMemory, &MapMemory};
  ScopedEntryPoint<PFN_vkUnmapMemory> m_unmap{vkUnmapMemory, &UnmapMemory};
};

VkPhysicalDeviceLimits Limits()
{
  VkPhysicalDeviceLimits limits = {};
  limits.minMemoryMapAlignment = 64;
  limits.nonCoherentAtomSize = 64;
  limits.minUniformBufferOffsetAlignment = 64;
  limits.minStorageBufferOffsetAlignment = 64;
  limits.maxMemoryAllocationCount = 1024;
  return limits;
}

VkPhysicalDeviceMemoryProperties MemoryProperties()
{
  VkPhysicalDeviceMemoryProperties properties = {};
  properties.memoryTypeCount = 1;
  properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  properties.memoryHeapCount = 1;
  return properties;
}

class Context : public dp::vulkan::VulkanBaseContext
{
public:
  Context(ref_ptr<dp::vulkan::VulkanObjectManager> manager, bool supportsTimeout)
    : VulkanBaseContext(VK_NULL_HANDLE, VK_NULL_HANDLE, {}, VK_NULL_HANDLE, 0, manager, nullptr, false, supportsTimeout)
  {
    m_surface = Handle<VkSurfaceKHR>(1);
    m_swapchain = Handle<VkSwapchainKHR>(2);
    for (uint32_t i = 0; i < dp::vulkan::kMaxInflightFrames; ++i)
    {
      m_fences[i] = Handle<VkFence>(10 + i);
      m_acquireSemaphores[i] = Handle<VkSemaphore>(20 + i);
      m_memoryCommandBuffers[i] = Handle<VkCommandBuffer>(30 + i);
      m_renderingCommandBuffers[i] = Handle<VkCommandBuffer>(40 + i);
    }
    m_swapchainImages.resize(3);
    CreateRenderSemaphores();
  }

  ~Context() override
  {
    // These handles are borrowed sentinels; presentation semaphores and staging buffers use mocked allocation.
    m_swapchain = VK_NULL_HANDLE;
    m_fences = {};
    m_acquireSemaphores = {};
    m_memoryCommandBuffers = {};
    m_renderingCommandBuffers = {};
  }

  void MakeCurrent() override {}
  void StartRenderPass() { m_isActiveRenderPass = true; }
  VkFence FrameFence() const { return m_fences[m_inflightFrameIndex]; }
  VkSemaphore AcquireSemaphore() const { return m_acquireSemaphores[m_inflightFrameIndex]; }

  void RecreatePresentationSemaphores(uint32_t imageCount)
  {
    DestroyRenderSemaphores();
    m_swapchainImages.resize(imageCount);
    CreateRenderSemaphores();
  }
};

UNIT_TEST(VulkanFrame_AcquisitionTimeoutPreservesFrameAndSynchronization)
{
  VulkanCalls calls;
  Context context(nullptr, true);
  auto const createdSemaphores = g_calls.m_createdSemaphores.size();
  unsigned completedFrames = 0;
  context.RegisterHandler(dp::vulkan::VulkanBaseContext::HandlerType::PostPresent,
                          [&completedFrames](uint32_t) { ++completedFrames; });

  for (unsigned attempt = 0; attempt < 3; ++attempt)
    TEST(context.BeginRendering() == dp::FrameStatus::RetryLater, ());

  TEST_EQUAL(g_calls.m_acquires, 3, ());
  TEST_EQUAL(g_calls.m_acquireTimeout, 250'000'000, ());
  TEST_EQUAL(g_calls.m_idleWaits, 0, ());
  TEST_EQUAL(g_calls.m_fenceResets, 0, ());
  TEST_EQUAL(g_calls.m_commandBegins, 0, ());
  TEST_EQUAL(context.GetCurrentFrameIndex(), 0, ());
  TEST_EQUAL(context.GetCurrentInflightFrameIndex(), 0, ());
  TEST_EQUAL(completedFrames, 0, ());
  TEST_EQUAL(g_calls.m_createdSemaphores.size(), createdSemaphores, ());
  TEST(g_calls.m_destroyedSemaphores.empty(), ());
}

UNIT_TEST(VulkanFrame_LegacyAcquisitionRetainsInfiniteTimeout)
{
  VulkanCalls calls;
  Context context(nullptr, false);
  TEST(context.BeginRendering() == dp::FrameStatus::RetryLater, ());
  TEST_EQUAL(g_calls.m_acquireTimeout, std::numeric_limits<uint64_t>::max(), ());
}

UNIT_TEST(VulkanFrame_TimeoutThenSuccessSubmitsAndPresentsAcquiredImage)
{
  VulkanCalls calls;
  dp::vulkan::VulkanObjectManager manager(VK_NULL_HANDLE, Limits(), MemoryProperties(), 0);
  manager.RegisterThread(dp::vulkan::VulkanObjectManager::Frontend);
  Context context(make_ref(&manager), true);
  context.Init(dp::ApiVersion::Vulkan);
  auto const frameFence = context.FrameFence();
  auto const acquireSemaphore = context.AcquireSemaphore();
  auto const presentSemaphore = g_calls.m_createdSemaphores[2];

  TEST(context.BeginRendering() == dp::FrameStatus::RetryLater, ());
  g_calls.m_acquireResult = VK_SUCCESS;
  TEST(context.BeginRendering() == dp::FrameStatus::Ready, ());
  TEST_EQUAL(g_calls.m_fenceResets, 1, ());
  TEST(g_calls.m_resetFence == frameFence, ());
  TEST_EQUAL(g_calls.m_commandBegins, 2, ());
  TEST_EQUAL(context.GetCurrentFrameIndex(), 1, ());

  context.StartRenderPass();
  context.EndRendering();
  context.Present();
  TEST(g_calls.m_acquireSemaphore == acquireSemaphore, ());
  TEST(g_calls.m_submitWaitSemaphore == acquireSemaphore, ());
  TEST(g_calls.m_submitSignalSemaphore == presentSemaphore, ());
  TEST(g_calls.m_presentWaitSemaphore == presentSemaphore, ());
  TEST_EQUAL(g_calls.m_presentImage, 2, ());
  TEST_EQUAL(context.GetCurrentInflightFrameIndex(), 1, ());
}

UNIT_TEST(VulkanFrame_PresentationSemaphoresFollowImageCount)
{
  VulkanCalls calls;
  Context context(nullptr, true);
  auto const previous = g_calls.m_createdSemaphores;
  TEST_EQUAL(previous.size(), 3, ());
  context.RecreatePresentationSemaphores(4);
  TEST(g_calls.m_destroyedSemaphores == previous, ());
  TEST_EQUAL(g_calls.m_createdSemaphores.size(), 7, ());
}
}  // namespace vulkan_frame_tests
