#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
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
  uint32_t m_imageCount = 3;
  VkResult m_acquireResult = VK_TIMEOUT;
  VkResult m_fenceResult = VK_SUCCESS;
  VkResult m_submitResult = VK_SUCCESS;
  VkResult m_presentResult = VK_SUCCESS;
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
  VkFence m_presentFence = VK_NULL_HANDLE;
  std::vector<VkFence> m_createdFences;
  std::map<VkFence, VkResult> m_fenceStatus;
  std::vector<std::string> m_retirement;
  std::vector<VkSemaphore> m_createdSemaphores;
  std::vector<VkSemaphore> m_destroyedSemaphores;
} g_calls;

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice)
{
  ++g_calls.m_idleWaits;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(VkDevice, uint32_t count, VkFence const * fences, VkBool32, uint64_t)
{
  if (g_calls.m_fenceStatus.contains(fences[0]))
  {
    g_calls.m_retirement.push_back("fences waited");
    for (uint32_t i = 0; i < count; ++i)
      g_calls.m_fenceStatus[fences[i]] = VK_SUCCESS;
  }
  return g_calls.m_fenceResult;
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
  if (g_calls.m_fenceStatus.contains(fences[0]))
    g_calls.m_fenceStatus[fences[0]] = VK_NOT_READY;
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
  return g_calls.m_submitResult;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresent(VkQueue, VkPresentInfoKHR const * info)
{
  TEST_EQUAL(info->waitSemaphoreCount, 1, ());
  TEST_EQUAL(info->swapchainCount, 1, ());
  g_calls.m_presentWaitSemaphore = info->pWaitSemaphores[0];
  g_calls.m_presentImage = info->pImageIndices[0];
  g_calls.m_presentFence = VK_NULL_HANDLE;
  if (info->pNext)
  {
    auto const * fences = static_cast<VkSwapchainPresentFenceInfoKHR const *>(info->pNext);
    TEST_EQUAL(fences->sType, VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR, ());
    TEST_EQUAL(fences->swapchainCount, 1, ());
    g_calls.m_presentFence = fences->pFences[0];
  }
  return g_calls.m_presentResult;
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
  g_calls.m_retirement.push_back("semaphore destroyed");
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchain(VkDevice, VkSwapchainCreateInfoKHR const *,
                                               VkAllocationCallbacks const *, VkSwapchainKHR * swapchain)
{
  *swapchain = Handle<VkSwapchainKHR>(g_calls.m_nextHandle++);
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroySwapchain(VkDevice, VkSwapchainKHR, VkAllocationCallbacks const *)
{
  g_calls.m_retirement.push_back("swapchain destroyed");
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImages(VkDevice, VkSwapchainKHR, uint32_t * count, VkImage * images)
{
  if (images)
    for (uint32_t i = 0; i < g_calls.m_imageCount; ++i)
      images[i] = Handle<VkImage>(g_calls.m_nextHandle++);
  *count = g_calls.m_imageCount;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice, VkImageViewCreateInfo const *, VkAllocationCallbacks const *,
                                               VkImageView * view)
{
  *view = Handle<VkImageView>(g_calls.m_nextHandle++);
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroyImageView(VkDevice, VkImageView, VkAllocationCallbacks const *) {}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice, VkFenceCreateInfo const *, VkAllocationCallbacks const *,
                                           VkFence * fence)
{
  *fence = Handle<VkFence>(g_calls.m_nextHandle++);
  g_calls.m_createdFences.push_back(*fence);
  g_calls.m_fenceStatus[*fence] = VK_NOT_READY;
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice, VkFence fence, VkAllocationCallbacks const *)
{
  g_calls.m_fenceStatus.erase(fence);
}

VKAPI_ATTR VkResult VKAPI_CALL FenceStatus(VkDevice, VkFence fence)
{
  return g_calls.m_fenceStatus.at(fence);
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
  ScopedEntryPoint<PFN_vkCreateSwapchainKHR> m_createSwapchain{vkCreateSwapchainKHR, &CreateSwapchain};
  ScopedEntryPoint<PFN_vkDestroySwapchainKHR> m_destroySwapchain{vkDestroySwapchainKHR, &DestroySwapchain};
  ScopedEntryPoint<PFN_vkGetSwapchainImagesKHR> m_images{vkGetSwapchainImagesKHR, &GetSwapchainImages};
  ScopedEntryPoint<PFN_vkCreateImageView> m_createView{vkCreateImageView, &CreateImageView};
  ScopedEntryPoint<PFN_vkDestroyImageView> m_destroyView{vkDestroyImageView, &DestroyImageView};
  ScopedEntryPoint<PFN_vkCreateFence> m_createFence{vkCreateFence, &CreateFence};
  ScopedEntryPoint<PFN_vkDestroyFence> m_destroyFence{vkDestroyFence, &DestroyFence};
  ScopedEntryPoint<PFN_vkGetFenceStatus> m_fenceStatus{vkGetFenceStatus, &FenceStatus};
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

struct CheckFailure
{
  std::string m_message;
};

bool CaptureCheck(base::SrcPoint const &, std::string const & message)
{
  throw CheckFailure{message};
}

void ExpectCheck(auto && action, char const * method)
{
  std::string message;
  {
    ScopedEntryPoint<base::AssertFailedFn> check(base::OnAssertFailed, &CaptureCheck);
    try
    {
      action();
    }
    catch (CheckFailure const & failure)
    {
      message = failure.m_message;
    }
  }
  TEST(!message.empty(), (method));
  TEST(message.find(method) != std::string::npos, (message));
}

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

// Exercise the acquisition contract of both stacked PRs independently of frame scheduling.
template <typename BaseContext>
class FrameContext : public BaseContext
{
public:
  FrameContext(ref_ptr<dp::vulkan::VulkanObjectManager> manager, bool supportsTimeout)
    : BaseContext(VK_NULL_HANDLE, VK_NULL_HANDLE, VkPhysicalDeviceProperties{}, VK_NULL_HANDLE, 0, manager, nullptr,
                  false, AcquireArgument(supportsTimeout))
  {
    this->m_surface = Handle<VkSurfaceKHR>(1);
    this->m_surfaceFormat = VkSurfaceFormatKHR{VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    this->m_surfaceCapabilities.minImageCount = 1;
    this->m_surfaceCapabilities.currentExtent = {640, 480};
    this->m_surfaceCapabilities.supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    this->m_surfaceCapabilities.supportedCompositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    for (uint32_t i = 0; i < dp::vulkan::kMaxInflightFrames; ++i)
    {
      this->m_fences[i] = Handle<VkFence>(10 + i);
      this->m_acquireSemaphores[i] = Handle<VkSemaphore>(20 + i);
      this->m_memoryCommandBuffers[i] = Handle<VkCommandBuffer>(30 + i);
      this->m_renderingCommandBuffers[i] = Handle<VkCommandBuffer>(40 + i);
    }
    this->RecreateSwapchain();
  }

  ~FrameContext() override
  {
    // These handles are borrowed sentinels; presentation semaphores and staging buffers use mocked allocation.
    this->m_fences = {};
    this->m_acquireSemaphores = {};
    this->m_memoryCommandBuffers = {};
    this->m_renderingCommandBuffers = {};
  }

  void MakeCurrent() override {}
  void StartRenderPass() { this->m_isActiveRenderPass = true; }
  VkFence FrameFence() const { return this->m_fences[this->m_inflightFrameIndex]; }
  VkSemaphore AcquireSemaphore() const { return this->m_acquireSemaphores[this->m_inflightFrameIndex]; }

  void RecreatePresentationSemaphores(uint32_t imageCount)
  {
    g_calls.m_imageCount = imageCount;
    this->RecreateSwapchain();
  }

private:
  static auto AcquireArgument(bool finite)
  {
    if constexpr (requires(FrameContext & context) { context.m_imageAcquireTimeoutNanoseconds; })
      return finite ? uint64_t{250'000'000} : std::numeric_limits<uint64_t>::max();
    else
      return finite;
  }
};

class MaintenanceContext : public dp::vulkan::VulkanBaseContext
{
public:
  template <typename... Args>
  explicit MaintenanceContext(Args &&... args) : VulkanBaseContext(std::forward<Args>(args)..., true)
  {}
};

using Context = FrameContext<dp::vulkan::VulkanBaseContext>;

bool Ready(auto status)
{
  if constexpr (std::is_same_v<decltype(status), bool>)
    return status;
  else
    return status == decltype(status)::Ready;
}

bool RetryLater(auto status)
{
  if constexpr (std::is_same_v<decltype(status), bool>)
    return !status;
  else
    return status == decltype(status)::RetryLater;
}

UNIT_TEST(VulkanFrame_AcquisitionTimeoutPreservesFrameAndSynchronization)
{
  VulkanCalls calls;
  Context context(nullptr, true);
  auto const createdSemaphores = g_calls.m_createdSemaphores.size();
  unsigned completedFrames = 0;
  context.RegisterHandler(dp::vulkan::VulkanBaseContext::HandlerType::PostPresent,
                          [&completedFrames](uint32_t) { ++completedFrames; });

  for (unsigned attempt = 0; attempt < 3; ++attempt)
    TEST(RetryLater(context.BeginRendering()), ());

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
  TEST(RetryLater(context.BeginRendering()), ());
  TEST_EQUAL(g_calls.m_acquireTimeout, std::numeric_limits<uint64_t>::max(), ());
}

UNIT_TEST(VulkanFrame_SurfaceLossPreservesSynchronization)
{
  VulkanCalls calls;
  Context context(nullptr, true);
  auto const frameFence = context.FrameFence();
  auto const acquireSemaphore = context.AcquireSemaphore();
  auto const semaphores = g_calls.m_createdSemaphores;
  g_calls.m_acquireResult = VK_ERROR_SURFACE_LOST_KHR;

  for (unsigned attempt = 0; attempt < 3; ++attempt)
    TEST(RetryLater(context.BeginRendering()), ());

  TEST_EQUAL(g_calls.m_idleWaits, 0, ());
  TEST_EQUAL(g_calls.m_fenceResets, 0, ());
  TEST(context.FrameFence() == frameFence, ());
  TEST(context.AcquireSemaphore() == acquireSemaphore, ());
  TEST(g_calls.m_createdSemaphores == semaphores, ());
  TEST(g_calls.m_destroyedSemaphores.empty(), ());
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

  TEST(RetryLater(context.BeginRendering()), ());
  g_calls.m_acquireResult = VK_SUCCESS;
  TEST(Ready(context.BeginRendering()), ());
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

UNIT_TEST(VulkanFrame_PresentationSurfaceLossCompletesFrameAndRemainsRetryable)
{
  VulkanCalls calls;
  dp::vulkan::VulkanObjectManager manager(VK_NULL_HANDLE, Limits(), MemoryProperties(), 0);
  manager.RegisterThread(dp::vulkan::VulkanObjectManager::Frontend);
  Context context(make_ref(&manager), true);
  context.Init(dp::ApiVersion::Vulkan);
  g_calls.m_acquireResult = VK_SUCCESS;
  TEST(Ready(context.BeginRendering()), ());
  context.StartRenderPass();
  context.EndRendering();
  g_calls.m_presentResult = VK_ERROR_SURFACE_LOST_KHR;
  context.Present();
  TEST_EQUAL(g_calls.m_presentImage, 2, ());
  TEST_EQUAL(context.GetCurrentInflightFrameIndex(), 1, ());

  g_calls.m_acquireResult = VK_ERROR_SURFACE_LOST_KHR;
  TEST(RetryLater(context.BeginRendering()), ());
  TEST_EQUAL(g_calls.m_idleWaits, 0, ());
}

UNIT_TEST(VulkanFrame_DeviceLossReportedAtFenceWait)
{
  VulkanCalls calls;
  Context context(nullptr, true);
  g_calls.m_fenceResult = VK_ERROR_DEVICE_LOST;
  ExpectCheck([&context] { context.BeginRendering(); }, "vkWaitForFences");
  TEST_EQUAL(g_calls.m_acquires, 0, ());
}

UNIT_TEST(VulkanFrame_DeviceLossReportedAtSubmissionAndPresentation)
{
  for (bool const submission : {true, false})
  {
    VulkanCalls calls;
    dp::vulkan::VulkanObjectManager manager(VK_NULL_HANDLE, Limits(), MemoryProperties(), 0);
    manager.RegisterThread(dp::vulkan::VulkanObjectManager::Frontend);
    Context context(make_ref(&manager), true);
    context.Init(dp::ApiVersion::Vulkan);
    g_calls.m_acquireResult = VK_SUCCESS;
    TEST(Ready(context.BeginRendering()), ());
    context.StartRenderPass();
    if (submission)
    {
      g_calls.m_submitResult = VK_ERROR_DEVICE_LOST;
      ExpectCheck([&context] { context.EndRendering(); }, "vkQueueSubmit");
    }
    else
    {
      context.EndRendering();
      g_calls.m_presentResult = VK_ERROR_DEVICE_LOST;
      ExpectCheck([&context] { context.Present(); }, "vkQueuePresentKHR");
    }
  }
}

UNIT_TEST(VulkanFrame_PresentationFencesReuseOnlyCompletedOperationsAndRetireResources)
{
  VulkanCalls calls;
  dp::vulkan::VulkanObjectManager manager(VK_NULL_HANDLE, Limits(), MemoryProperties(), 0);
  manager.RegisterThread(dp::vulkan::VulkanObjectManager::Frontend);
  FrameContext<MaintenanceContext> context(make_ref(&manager), true);
  context.Init(dp::ApiVersion::Vulkan);
  auto draw = [&context]
  {
    g_calls.m_acquireResult = VK_SUCCESS;
    TEST(Ready(context.BeginRendering()), ());
    context.StartRenderPass();
    context.EndRendering();
    context.Present();
    TEST(g_calls.m_presentFence != VK_NULL_HANDLE, ());
  };
  draw();
  auto const first = g_calls.m_presentFence;
  draw();
  TEST(g_calls.m_presentFence != first, ());
  TEST_EQUAL(g_calls.m_createdFences.size(), 2, ());
  g_calls.m_fenceStatus[first] = VK_SUCCESS;
  draw();
  TEST(g_calls.m_presentFence == first, ());
  TEST_EQUAL(g_calls.m_createdFences.size(), 2, ());

  g_calls.m_retirement.clear();
  context.RecreatePresentationSemaphores(4);
  auto const waited = std::find(g_calls.m_retirement.begin(), g_calls.m_retirement.end(), "fences waited");
  auto const semaphore = std::find(g_calls.m_retirement.begin(), g_calls.m_retirement.end(), "semaphore destroyed");
  auto const swapchain = std::find(g_calls.m_retirement.begin(), g_calls.m_retirement.end(), "swapchain destroyed");
  TEST(waited != g_calls.m_retirement.end(), ());
  TEST(waited < semaphore, ());
  TEST(waited < swapchain, ());
  TEST(g_calls.m_fenceStatus.empty(), ());
}

UNIT_TEST(VulkanFrame_UnextendedPresentationUsesNoPresentationFences)
{
  VulkanCalls calls;
  dp::vulkan::VulkanObjectManager manager(VK_NULL_HANDLE, Limits(), MemoryProperties(), 0);
  manager.RegisterThread(dp::vulkan::VulkanObjectManager::Frontend);
  Context context(make_ref(&manager), true);
  context.Init(dp::ApiVersion::Vulkan);
  g_calls.m_acquireResult = VK_SUCCESS;
  TEST(Ready(context.BeginRendering()), ());
  context.StartRenderPass();
  context.EndRendering();
  context.Present();
  TEST(g_calls.m_presentFence == VK_NULL_HANDLE, ());
  TEST(g_calls.m_createdFences.empty(), ());
}
}  // namespace vulkan_frame_tests
