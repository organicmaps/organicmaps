#include "std/target_os.hpp"
#if !defined(OMIM_OS_LINUX)
#error Unsupported OS
#endif

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <dlfcn.h>

#include <vulkan_wrapper.h>

#include "drape/vulkan/vulkan_context_factory.hpp"

#include "drape/gl_functions.hpp"
#include "drape/gl_includes.hpp"
#include "drape/oglcontext.hpp"

#include <array>

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

    VkResult statusCode;
    CHECK(vkCreateWaylandSurfaceKHR, ());
    statusCode = vkCreateWaylandSurfaceKHR(m_vulkanInstance, &createInfo, nullptr, &m_surface);
    if (statusCode != VK_SUCCESS)
    {
      LOG_ERROR_VK_CALL(vkCreateWaylandSurfaceKHR, statusCode);
      return;
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    m_framebufferWidth = static_cast<uint32_t>(fbWidth);
    m_framebufferHeight = static_cast<uint32_t>(fbHeight);

    FinishSetup();
  }

  void ResetSurface()
  {
    if (m_drawContext)
      m_drawContext->ResetSurface(false);

    vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
  }

private:
  void FinishSetup()
  {
    uint32_t const renderingQueueIndex = m_drawContext->GetRenderingQueueFamilyIndex();
    VkBool32 supportsPresent;
    auto statusCode = vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, renderingQueueIndex, m_surface, &supportsPresent);
    if (statusCode != VK_SUCCESS)
    {
      LOG_ERROR_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR, statusCode);
      return;
    }
    if (supportsPresent != VK_TRUE)
    {
      LOG(LERROR, ("Present is not supported on the rendering queue family"));
      return;
    }

    if (!QuerySurfaceSize())
    {
      LOG(LERROR, ("Failed to query surface size/format"));
      return;
    }

    // On Wayland, currentExtent may be {UINT32_MAX, UINT32_MAX} meaning the surface
    // extent is not predetermined. Replace with the actual framebuffer size.
    if (m_surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
      m_surfaceCapabilities.currentExtent.width = m_framebufferWidth;
      m_surfaceCapabilities.currentExtent.height = m_framebufferHeight;
    }

    if (m_drawContext)
      m_drawContext->SetSurface(m_surface, m_surfaceFormat, m_surfaceCapabilities);
  }

  uint32_t m_framebufferWidth = 0;
  uint32_t m_framebufferHeight = 0;
};

struct EglFunctions
{
  EglFunctions()
  {
    std::array<char const *, 2> libs = {
        "libEGL.so.1",
        "libEGL.so",
    };

    for (char const * lib : libs)
    {
      m_module = dlopen(lib, RTLD_LAZY | RTLD_LOCAL);
      if (m_module)
        break;
    }

    CHECK(m_module != nullptr, ("Failed to load EGL"));

    eglGetCurrentContext = loadFunction<PFNEGLGETCURRENTCONTEXTPROC>("eglGetCurrentContext");
    eglChooseConfig = loadFunction<PFNEGLCHOOSECONFIGPROC>("eglChooseConfig");
    eglCreateContext = loadFunction<PFNEGLCREATECONTEXTPROC>("eglCreateContext");
    eglDestroyContext = loadFunction<PFNEGLDESTROYCONTEXTPROC>("eglDestroyContext");
    eglMakeCurrent = loadFunction<PFNEGLMAKECURRENTPROC>("eglMakeCurrent");
    eglCreatePbufferSurface = loadFunction<PFNEGLCREATEPBUFFERSURFACEPROC>("eglCreatePbufferSurface");
    eglDestroySurface = loadFunction<PFNEGLDESTROYSURFACEPROC>("eglDestroySurface");
    eglGetError = loadFunction<PFNEGLGETERRORPROC>("eglGetError");
  }

  ~EglFunctions()
  {
    if (m_module)
      dlclose(m_module);
  }

  EglFunctions(EglFunctions const &) = delete;
  EglFunctions & operator=(EglFunctions const &) = delete;

  PFNEGLGETCURRENTCONTEXTPROC eglGetCurrentContext = nullptr;
  PFNEGLCHOOSECONFIGPROC eglChooseConfig = nullptr;
  PFNEGLCREATECONTEXTPROC eglCreateContext = nullptr;
  PFNEGLDESTROYCONTEXTPROC eglDestroyContext = nullptr;
  PFNEGLMAKECURRENTPROC eglMakeCurrent = nullptr;
  PFNEGLCREATEPBUFFERSURFACEPROC eglCreatePbufferSurface = nullptr;
  PFNEGLDESTROYSURFACEPROC eglDestroySurface = nullptr;
  PFNEGLGETERRORPROC eglGetError = nullptr;

private:
  template <typename T>
  T loadFunction(char const * func)
  {
    auto f = reinterpret_cast<T>(dlsym(m_module, func));
    ASSERT(f, ("Failed to load EGL function:", func));
    return f;
  }

  void * m_module = nullptr;
};

class LinuxEglContext : public dp::OGLContext
{
public:
  explicit LinuxEglContext(GLFWwindow * window) : m_glfwWindow(window) {}

  LinuxEglContext(EglFunctions const & egl, GLFWwindow * window) : m_egl(&egl)
  {
    glfwMakeContextCurrent(window);

    auto display = glfwGetEGLDisplay();
    auto shareContext = m_egl->eglGetCurrentContext();

    if (shareContext == EGL_NO_CONTEXT)
    {
      LOG(LWARNING, ("EGL: no current context to share with"));
      return;
    }

    EGLint fbcount = 0;
    EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
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

    if (!m_egl->eglChooseConfig(display, configAttribs, &m_config, 1, &fbcount) || fbcount == 0)
    {
      LOG(LWARNING, ("EGL: upload context failed to choose config"));
      return;
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};

    m_context = m_egl->eglCreateContext(display, m_config, shareContext, contextAttribs);

    if (m_context == EGL_NO_CONTEXT)
    {
      LOG(LWARNING, ("EGL: upload context failed to create context, error:", m_egl->eglGetError()));
      return;
    }

    EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    m_pbufferSurface = m_egl->eglCreatePbufferSurface(display, m_config, pbufferAttribs);
    if (m_pbufferSurface == EGL_NO_SURFACE)
    {
      LOG(LWARNING, ("EGL: upload context failed to create pbuffer surface, error:", m_egl->eglGetError()));
      return;
    }

    m_display = display;
  }

  ~LinuxEglContext() override
  {
    if (!m_egl)
      return;

    if (m_pbufferSurface != EGL_NO_SURFACE)
    {
      m_egl->eglDestroySurface(m_display, m_pbufferSurface);
      m_pbufferSurface = EGL_NO_SURFACE;
    }
    if (m_context != EGL_NO_CONTEXT)
    {
      m_egl->eglDestroyContext(m_display, m_context);
      m_context = EGL_NO_CONTEXT;
    }
  }

  void Present() override
  {
    if (m_glfwWindow)
      glfwSwapBuffers(m_glfwWindow);
  }

  void MakeCurrent() override
  {
    if (m_glfwWindow && !m_egl)
    {
      glfwMakeContextCurrent(m_glfwWindow);
      return;
    }
    if (m_egl && m_context != EGL_NO_CONTEXT)
    {
      if (!m_egl->eglMakeCurrent(m_display, m_pbufferSurface, m_pbufferSurface, m_context))
        LOG(LWARNING, ("EGL: upload context MakeCurrent failed, error:", m_egl->eglGetError()));
    }
  }

  void DoneCurrent() override
  {
    if (m_glfwWindow && !m_egl)
    {
      glfwMakeContextCurrent(nullptr);
      return;
    }
    if (m_egl && m_context != EGL_NO_CONTEXT)
    {
      if (!m_egl->eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
        LOG(LWARNING, ("EGL: upload context DoneCurrent failed, error:", m_egl->eglGetError()));
    }
  }

  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override
  {
    if (framebuffer)
      framebuffer->Bind();
    else
      GLFunctions::glBindFramebuffer(0);
  }

private:
  GLFWwindow * m_glfwWindow = nullptr;

  EglFunctions const * m_egl = nullptr;
  EGLDisplay m_display = EGL_NO_DISPLAY;
  EGLConfig m_config = nullptr;
  EGLContext m_context = EGL_NO_CONTEXT;
  EGLSurface m_pbufferSurface = EGL_NO_SURFACE;
};

class LinuxEglContextFactory : public dp::GraphicsContextFactory
{
public:
  explicit LinuxEglContextFactory(GLFWwindow * window) : m_glfwWindow(window) {}

  dp::GraphicsContext * GetDrawContext() override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    if (m_drawContext == nullptr)
      m_drawContext = std::make_unique<LinuxEglContext>(m_glfwWindow);
    return m_drawContext.get();
  }

  dp::GraphicsContext * GetResourcesUploadContext() override
  {
    std::lock_guard<std::mutex> lock(m_contextAccess);
    if (m_uploadContext == nullptr)
      m_uploadContext = std::make_unique<LinuxEglContext>(m_egl, m_glfwWindow);
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
  EglFunctions m_egl;

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
