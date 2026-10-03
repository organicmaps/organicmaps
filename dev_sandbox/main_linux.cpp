#include "std/target_os.hpp"
#if !defined(OMIM_OS_LINUX)
#error Unsupported OS
#endif

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_EGL
#define WL_EGL_PLATFORM
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <EGL/egl.h>
#include <wayland-egl.h>

#include <vulkan_wrapper.h>

#include <limits>

#include "drape/vulkan/vulkan_context_factory.hpp"

#include "drape/gl_functions.hpp"
#include "drape/gl_includes.hpp"
#include "drape/oglcontext.hpp"

class LinuxVulkanContextFactory : public dp::vulkan::VulkanContextFactory
{
public:
  LinuxVulkanContextFactory() : dp::vulkan::VulkanContextFactory(1, 33, false) {}

  void SetSurfaceWayland(wl_display * display, wl_surface * surface, GLFWwindow * window)
  {
    VkWaylandSurfaceCreateInfoKHR const createInfo = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .display = display,
        .surface = surface,
    };

    CHECK_VK_CALL(vkCreateWaylandSurfaceKHR(m_vulkanInstance, &createInfo, nullptr, &m_surface));

    uint32_t const renderingQueueIndex = m_drawContext->GetRenderingQueueFamilyIndex();
    VkBool32 supportsPresent;
    CHECK_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, renderingQueueIndex, m_surface, &supportsPresent));
    CHECK_EQUAL(supportsPresent, VK_TRUE, ());

    CHECK(QuerySurfaceSize(), ());

    // On Wayland, currentExtent may be {UINT32_MAX, UINT32_MAX} meaning the surface
    // extent is not predetermined. Replace with the actual framebuffer size.
    if (m_surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
      int fbWidth = 0, fbHeight = 0;
      glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
      m_surfaceCapabilities.currentExtent.width = static_cast<uint32_t>(fbWidth);
      m_surfaceCapabilities.currentExtent.height = static_cast<uint32_t>(fbHeight);
    }
    m_surfaceWidth = static_cast<int>(m_surfaceCapabilities.currentExtent.width);
    m_surfaceHeight = static_cast<int>(m_surfaceCapabilities.currentExtent.height);

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

class LinuxEglContext : public dp::OGLContext
{
public:
  LinuxEglContext(EGLDisplay display, GLFWwindow * window, LinuxEglContext * contextToShareWith, bool isUploadContext)
    : m_display(display)
    , m_isUploadContext(isUploadContext)
  {
    EGLint fbcount = 0;
    EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              24,
                              EGL_STENCIL_SIZE,
                              8,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_NONE};

    if (!eglChooseConfig(display, configAttribs, &m_config, 1, &fbcount) || fbcount == 0)
    {
      LOG(LWARNING, ("EGL: failed to choose config, error:", eglGetError()));
      return;
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};

    m_context = eglCreateContext(display, m_config, contextToShareWith ? contextToShareWith->m_context : EGL_NO_CONTEXT,
                                 contextAttribs);

    if (m_context == EGL_NO_CONTEXT)
    {
      LOG(LWARNING, ("EGL: failed to create context, error:", eglGetError()));
      return;
    }

    if (m_isUploadContext)
    {
      EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
      m_surface = eglCreatePbufferSurface(display, m_config, pbufferAttribs);
    }
    else
    {
      int fbWidth = 0, fbHeight = 0;
      glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
      m_wlWindow = wl_egl_window_create(glfwGetWaylandWindow(window), fbWidth, fbHeight);
      m_surface = eglCreateWindowSurface(display, m_config, m_wlWindow, nullptr);
    }
    if (m_surface == EGL_NO_SURFACE)
    {
      LOG(LWARNING, ("EGL: failed to create surface, error:", eglGetError()));
      return;
    }
  }

  ~LinuxEglContext() override
  {
    if (m_surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(m_display, m_surface);
      m_surface = EGL_NO_SURFACE;
    }
    if (m_wlWindow)
    {
      wl_egl_window_destroy(m_wlWindow);
      m_wlWindow = nullptr;
    }
    if (m_context != EGL_NO_CONTEXT)
    {
      eglDestroyContext(m_display, m_context);
      m_context = EGL_NO_CONTEXT;
    }
  }

  void Present() override
  {
    if (!m_isUploadContext && !eglSwapBuffers(m_display, m_surface))
      LOG(LWARNING, ("EGL: eglSwapBuffers failed, error:", eglGetError()));
  }

  void MakeCurrent() override
  {
    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context))
      LOG(LWARNING, ("EGL: MakeCurrent failed, error:", eglGetError()));
  }

  void DoneCurrent() override
  {
    if (!eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
      LOG(LWARNING, ("EGL: DoneCurrent failed, error:", eglGetError()));
  }

  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override
  {
    if (framebuffer)
      framebuffer->Bind();
    else
      GLFunctions::glBindFramebuffer(0);
  }

private:
  EGLDisplay m_display = EGL_NO_DISPLAY;
  wl_egl_window * m_wlWindow = nullptr;
  EGLConfig m_config = nullptr;
  EGLContext m_context = EGL_NO_CONTEXT;
  EGLSurface m_surface = EGL_NO_SURFACE;

  bool m_isUploadContext = false;
};

class LinuxEglContextFactory : public dp::GraphicsContextFactory
{
public:
  explicit LinuxEglContextFactory(GLFWwindow * window) : m_glfwWindow(window)
  {
    m_display = eglGetDisplay(glfwGetWaylandDisplay());
    if (m_display == EGL_NO_DISPLAY)
      LOG(LERROR, ("EGL: eglGetDisplay failed"));
    else if (!eglInitialize(m_display, nullptr, nullptr))
      LOG(LERROR, ("EGL: eglInitialize failed, error:", eglGetError()));
    else if (!eglBindAPI(EGL_OPENGL_ES_API))
      LOG(LERROR, ("EGL: eglBindAPI failed, error:", eglGetError()));
  }

  ~LinuxEglContextFactory() override
  {
    if (m_display != EGL_NO_DISPLAY)
      eglTerminate(m_display);
  }

  dp::GraphicsContext * GetDrawContext() override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    if (m_drawContext == nullptr)
      m_drawContext = std::make_unique<LinuxEglContext>(m_display, m_glfwWindow, m_uploadContext.get(), false);
    return m_drawContext.get();
  }

  dp::GraphicsContext * GetResourcesUploadContext() override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    if (m_uploadContext == nullptr)
      m_uploadContext = std::make_unique<LinuxEglContext>(m_display, m_glfwWindow, m_drawContext.get(), true);
    return m_uploadContext.get();
  }

  void WaitForInitialization(dp::GraphicsContext *) override
  {
    std::unique_lock<std::mutex> lock(m_initializationMutex);
    if (m_isInitialized)
      return;

    m_initializationCounter++;
    if (m_initializationCounter >= kGLThreadsCount)
    {
      m_isInitialized = true;
      m_initializationCondition.notify_all();
    }
    else
    {
      m_initializationCondition.wait(lock, [this] { return m_isInitialized; });
    }
  }

  bool IsDrawContextCreated() const override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    return m_drawContext != nullptr;
  }

  bool IsUploadContextCreated() const override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    return m_uploadContext != nullptr;
  }

private:
  static size_t constexpr kGLThreadsCount = 2;

  GLFWwindow * m_glfwWindow;
  EGLDisplay m_display = EGL_NO_DISPLAY;

  std::unique_ptr<LinuxEglContext> m_drawContext;
  std::unique_ptr<LinuxEglContext> m_uploadContext;

  mutable std::mutex m_contextAccess;

  bool m_isInitialized = false;
  size_t m_initializationCounter = 0;
  std::condition_variable m_initializationCondition;
  std::mutex m_initializationMutex;
};

drape_ptr<dp::GraphicsContextFactory> CreateContextFactory(GLFWwindow * window, dp::ApiVersion api, m2::PointU size)
{
  if (api == dp::ApiVersion::Vulkan)
  {
    auto contextFactory = make_unique_dp<LinuxVulkanContextFactory>();
    contextFactory->SetSurfaceWayland(glfwGetWaylandDisplay(), glfwGetWaylandWindow(window), window);
    return contextFactory;
  }

  if (api == dp::ApiVersion::OpenGLES3)
    return make_unique_dp<LinuxEglContextFactory>(window);

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
  if (api == dp::ApiVersion::OpenGLES3)
  {
    // Do nothing
  }
  else if (api == dp::ApiVersion::Vulkan)
  {
    ref_ptr<LinuxVulkanContextFactory> linuxContextFactory = contextFactory;
    linuxContextFactory->ResetSurface();
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
