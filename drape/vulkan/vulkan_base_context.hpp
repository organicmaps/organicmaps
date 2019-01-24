#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/vulkan/vulkan_device_holder.hpp"
#include "drape/vulkan/vulkan_object_manager.hpp"

#include "geometry/point2d.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <boost/optional.hpp>

#include <cstdint>

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
                    ref_ptr<VulkanObjectManager> objectManager);

  void Present() override;
  void MakeCurrent() override {}
  void DoneCurrent() override {}
  bool Validate() override { return true; }
  void Resize(int w, int h) override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override {}
  void ApplyFramebuffer(std::string const & framebufferLabel) override {}
  void Init(ApiVersion apiVersion) override {}
  ApiVersion GetApiVersion() const override { return dp::ApiVersion::Vulkan; }
  std::string GetRendererName() const override;
  std::string GetRendererVersion() const override;

  void DebugSynchronizeWithCPU() override {}
  void PushDebugLabel(std::string const & label) override {}
  void PopDebugLabel() override {}
  
  void SetClearColor(Color const & color) override {}
  void Clear(uint32_t clearBits, uint32_t storeBits) override {}
  void Flush() override {}
  void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override {}
  void SetDepthTestEnabled(bool enabled) override {}
  void SetDepthTestFunction(TestFunction depthFunction) override {}
  void SetStencilTestEnabled(bool enabled) override {}
  void SetStencilFunction(StencilFace face, TestFunction stencilFunction) override {}
  void SetStencilActions(StencilFace face, StencilAction stencilFailAction,
                         StencilAction depthFailAction, StencilAction passAction) override {}
  void SetStencilReferenceValue(uint32_t stencilReferenceValue) override;

  void SetSurface(VkSurfaceKHR surface, VkFormat surfaceFormat, int width, int height);
  void ResetSurface();

  VkPhysicalDevice const GetPhysicalDevice() const { return m_gpu; }

  VkDevice GetDevice() const { return m_device; }
  DeviceHolderPtr GetDeviceHolder() const { return m_deviceHolder; }

  VkPhysicalDeviceProperties const & GetGpuProperties() const { return m_gpuProperties; }
  uint32_t GetRenderingQueueFamilyIndex() { return m_renderingQueueFamilyIndex; }

  ref_ptr<VulkanObjectManager> GetObjectManager() const { return m_objectManager; }

  VkCommandBuffer GetCurrentCommandBuffer() const { CHECK(false, ("Implement me")); return nullptr; }

protected:
  VkInstance const m_vulkanInstance;
  VkPhysicalDevice const m_gpu;
  VkPhysicalDeviceProperties const m_gpuProperties;
  VkDevice const m_device;
  uint32_t const m_renderingQueueFamilyIndex;
  ref_ptr<VulkanObjectManager> m_objectManager;

  std::shared_ptr<DeviceHolder> m_deviceHolder;

  boost::optional<VkSurfaceKHR> m_surface;

  uint32_t m_stencilReferenceValue = 1;
};
}  // namespace vulkan
}  // namespace dp
