#pragma once

#include "com/mapswithme/core/jni_helper.hpp"

#include "drape/graphics_context_factory.hpp"
#include "drape/vulkan/vulkan_base_context.hpp"
#include "drape/vulkan/vulkan_layers.hpp"
#include "drape/pointers.hpp"

#include <vulkan_wrapper.h>
#include <vulkan/vulkan.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

namespace android
{
class AndroidVulkanContextFactory : public dp::GraphicsContextFactory
{
public:
  AndroidVulkanContextFactory();
  ~AndroidVulkanContextFactory();

  bool IsVulkanSupported() const;
  bool IsValid() const;

  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override;
  bool IsUploadContextCreated() const override;
  void SetPresentAvailable(bool available) override;

  void SetSurface(JNIEnv * env, jobject jsurface);
  void ResetSurface();

  int GetWidth() const;
  int GetHeight() const;
  void UpdateSurfaceSize(int w, int h);

private:
  bool QuerySurfaceSize();

  VkInstance m_vulkanInstance = nullptr;
  drape_ptr<dp::vulkan::Layers> m_layers;
  VkPhysicalDevice m_gpu = nullptr;
  VkDevice m_device = nullptr;
  drape_ptr<dp::vulkan::VulkanBaseContext> m_drawContext;
  drape_ptr<dp::vulkan::VulkanBaseContext> m_uploadContext;

  ANativeWindow * m_nativeWindow = nullptr;
  bool m_windowSurfaceValid = false;
  VkSurfaceKHR m_surface = 0;
  VkFormat m_surfaceFormat = VK_FORMAT_UNDEFINED;
  int m_surfaceWidth = 0;
  int m_surfaceHeight = 0;
};
}  // namespace android
