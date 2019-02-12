#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_gpu_program.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"
#include "drape/vulkan/vulkan_pipeline.hpp"
#include "drape/vulkan/vulkan_utils.hpp"

#include "geometry/point2d.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <boost/optional.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace dp
{
namespace vulkan
{
class VulkanBaseContext : public dp::GraphicsContext
{
public:
  VulkanBaseContext(VkInstance vulkanInstance, VkPhysicalDevice gpu,
                    VkPhysicalDeviceProperties const & gpuProperties,
                    VkDevice device, uint32_t renderingQueueFamilyIndex,
                    VkFormat depthFormat, ref_ptr<VulkanObjectManager> objectManager,
                    drape_ptr<VulkanPipeline> && pipeline);
  ~VulkanBaseContext() override;

  using ContextHandler = std::function<void(ref_ptr<VulkanBaseContext>)>;

  void BeginRendering() override;
  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  bool Validate() override;
  void Resize(int w, int h) override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;
  void ApplyFramebuffer(std::string const & framebufferLabel) override;
  void Init(ApiVersion apiVersion) override;
  ApiVersion GetApiVersion() const override { return dp::ApiVersion::Vulkan; }
  std::string GetRendererName() const override;
  std::string GetRendererVersion() const override;

  void DebugSynchronizeWithCPU() override {}
  void PushDebugLabel(std::string const & label) override {}
  void PopDebugLabel() override {}
  
  void SetClearColor(Color const & color) override;
  void Clear(uint32_t clearBits, uint32_t storeBits) override;
  void Flush() override;
  void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
  void SetDepthTestEnabled(bool enabled) override;
  void SetDepthTestFunction(TestFunction depthFunction) override;
  void SetStencilTestEnabled(bool enabled) override;
  void SetStencilFunction(StencilFace face, TestFunction stencilFunction) override;
  void SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                         StencilAction depthFailAction, StencilAction passAction) override;
  void SetStencilReferenceValue(uint32_t stencilReferenceValue) override;

  void SetPrimitiveTopology(VkPrimitiveTopology topology);
  void SetBindingInfo(std::vector<dp::BindingInfo> const & bindingInfo);
  void SetProgram(ref_ptr<VulkanGpuProgram> program);
  void SetBlendingEnabled(bool blendingEnabled);

  void ApplyParamDescriptor(ParamDescriptor && descriptor);
  void ClearParamDescriptors();

  void SetSurface(VkSurfaceKHR surface, VkSurfaceFormatKHR surfaceFormat,
                  VkSurfaceCapabilitiesKHR surfaceCapabilities, int width, int height);
  void ResetSurface();

  VkPhysicalDevice const GetPhysicalDevice() const { return m_gpu; }

  VkDevice GetDevice() const { return m_device; }

  VkPhysicalDeviceProperties const & GetGpuProperties() const { return m_gpuProperties; }
  uint32_t GetRenderingQueueFamilyIndex() { return m_renderingQueueFamilyIndex; }

  ref_ptr<VulkanObjectManager> GetObjectManager() const { return m_objectManager; }

  VkCommandBuffer GetCurrentCommandBuffer() const { return m_commandBuffer; }

  VkPipeline GetCurrentPipeline();

  enum class HandlerType : uint8_t
  {
    PrePresent = 0,
    PostPresent,

    Count
  };
  uint32_t RegisterHandler(HandlerType handlerType, ContextHandler && handler);
  void UnregisterHandler(uint32_t id);

protected:
  void RecreateSwapchain();
  void DestroySwapchain();

  void CreateCommandPool();
  void DestroyCommandPool();

  void CreateCommandBuffer();
  void DestroyCommandBuffer();

  void CreateDepthTexture();
  void DestroyDepthTexture();

  void CreateDefaultFramebuffer();
  void DestroyDefaultFramebuffer();

  void CreateRenderPass();
  void DestroyRenderPass();

  VkInstance const m_vulkanInstance;
  VkPhysicalDevice const m_gpu;
  VkPhysicalDeviceProperties const m_gpuProperties;
  VkDevice const m_device;
  uint32_t const m_renderingQueueFamilyIndex;
  VkFormat const m_depthFormat;

  VkQueue m_queue;
  VkCommandPool m_commandPool;
  VkPipelineCache m_pipelineCache;
  VkSubmitInfo m_submitInfo;
  VkCommandBuffer m_commandBuffer;
  VkRenderPass m_renderPass;

  // Swap chain image presentation
  VkSemaphore m_presentComplete;
  // Command buffer submission and execution
  VkSemaphore m_renderComplete;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkFence m_fence;

  ref_ptr<VulkanObjectManager> m_objectManager;
  drape_ptr<VulkanPipeline> m_pipeline;
  boost::optional<VkSurfaceKHR> m_surface;

  VkSurfaceCapabilitiesKHR m_surfaceCapabilities;
  boost::optional<VkSurfaceFormatKHR> m_surfaceFormat;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  std::vector<VkImageView> m_swapchainImageViews;
  uint32_t m_imageIndex = 0;

  VulkanObject m_depthStencil;
  std::vector<VkFramebuffer> m_defaultFramebuffers;

  ref_ptr<dp::BaseFramebuffer> m_currentFramebuffer;

  std::array<std::vector<std::pair<uint32_t, ContextHandler>>,
             static_cast<size_t>(HandlerType::Count)> m_handlers;

  bool m_depthEnabled = false;
  bool m_stencilEnabled = false;
  StencilFace m_stencilFunctionFace = {};
  TestFunction m_stencilFunction = {};
  TestFunction m_depthFunction = {};
  StencilFace m_stencilActionFace = {};
  StencilAction m_stencilFailAction = {};
  StencilAction m_depthFailAction = {};
  StencilAction m_passAction = {};
  uint32_t m_stencilReferenceValue = 1;
};
}  // namespace vulkan
}  // namespace dp
