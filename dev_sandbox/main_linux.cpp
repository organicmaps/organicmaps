#include "dev_sandbox/context_factory.hpp"

#include "drape/gl_functions.hpp"
#include "drape/oglcontext.hpp"
#include "drape/vulkan/vulkan_context_factory.hpp"

#include "std/target_os.hpp"
#if !defined(OMIM_OS_LINUX)
#error Unsupported OS
#endif

// Vulkan types must precede GLFW's surface declarations.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <atomic>
#include <span>

class LinuxVulkanContextFactory : public dp::vulkan::VulkanContextFactory
{
public:
  explicit LinuxVulkanContextFactory(std::span<char const * const> extensions)
    : dp::vulkan::VulkanContextFactory(1, 33, false, extensions)
  {}

  void SetSurface(GLFWwindow * window, m2::PointU size)
  {
    CHECK(IsVulkanSupported(), ("Vulkan is not supported; cannot create the sandbox surface"));
    CHECK_VK_CALL(glfwCreateWindowSurface(m_vulkanInstance, window, nullptr, &m_surface));

    uint32_t const renderingQueueIndex = m_drawContext->GetRenderingQueueFamilyIndex();
    VkBool32 supportsPresent;
    CHECK_VK_CALL(vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, renderingQueueIndex, m_surface, &supportsPresent));
    CHECK_EQUAL(supportsPresent, VK_TRUE, ("Graphics queue cannot present to the GLFW surface"));
    CHECK(QuerySurfaceSize(size), ("Failed to query sandbox surface capabilities"));
    // Wayland mandates MAILBOX; FIFO can wait forever for a hidden window's frame callback.
    m_drawContext->SetSurface(m_surface, m_surfaceFormat, size, VK_PRESENT_MODE_MAILBOX_KHR);
  }

  void ResetSurface()
  {
    m_drawContext->ResetSurface(false);
    vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }
};

// Contexts borrow main-thread-owned windows. Drape releases them before joining.
class LinuxGLContext : public dp::OGLContext
{
public:
  LinuxGLContext(GLFWwindow * window, bool upload) : m_window(window), m_upload(upload)
  {
    CHECK(m_window, ("Missing GLFW OpenGL context"));
  }

  ~LinuxGLContext() override { ASSERT(!m_isCurrent, ("OpenGL worker did not release its context")); }

  dp::FrameStatus BeginRendering() override
  {
    return m_presentAvailable ? dp::FrameStatus::Ready : dp::FrameStatus::Suspended;
  }
  void SetPresentAvailable(bool available) override { m_presentAvailable = available; }

  void Present() override
  {
    ASSERT_EQUAL(glfwGetCurrentContext(), m_window, ());
    if (!m_upload)
      glfwSwapBuffers(m_window);
  }

  void MakeCurrent() override
  {
    ASSERT(!m_isCurrent, ("OpenGL context is already current on a worker"));
    ASSERT(!glfwGetCurrentContext(), ());
    glfwMakeContextCurrent(m_window);
    CHECK_EQUAL(glfwGetCurrentContext(), m_window, ("Failed to make GLFW OpenGL context current"));
    // Wayland can withhold frame callbacks for hidden windows, blocking vsynced EGL swaps and teardown.
    if (!m_upload)
      glfwSwapInterval(0);
    m_isCurrent = true;
  }

  void DoneCurrent() override
  {
    // Surface destruction and final renderer teardown both release the context.
    ASSERT(glfwGetCurrentContext() == m_window || (!glfwGetCurrentContext() && !m_isCurrent), ());
    glfwMakeContextCurrent(nullptr);
    CHECK(!glfwGetCurrentContext(), ("Failed to release GLFW OpenGL context"));
    m_isCurrent = false;
  }

  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override
  {
    ASSERT_EQUAL(glfwGetCurrentContext(), m_window, ());
    if (framebuffer)
      framebuffer->Bind();
    else
      GLFunctions::glBindFramebuffer(0);
  }

private:
  GLFWwindow * const m_window;
  bool const m_upload;
  std::atomic<bool> m_isCurrent = false;
  std::atomic<bool> m_presentAvailable = true;
};

class LinuxContextFactory : public dp::GraphicsContextFactory
{
public:
  explicit LinuxContextFactory(GlfwWindows const & windows)
    : m_drawContext(windows.m_visible, false)
    , m_uploadContext(windows.m_upload, true)
  {}

  dp::GraphicsContext * GetDrawContext() override { return &m_drawContext; }
  dp::GraphicsContext * GetResourcesUploadContext() override { return &m_uploadContext; }
  bool IsDrawContextCreated() const override { return true; }
  bool IsUploadContextCreated() const override { return true; }
  void SetPresentAvailable(bool available) override { m_drawContext.SetPresentAvailable(available); }

private:
  LinuxGLContext m_drawContext;
  LinuxGLContext m_uploadContext;
};

drape_ptr<dp::GraphicsContextFactory> CreateContextFactory(GlfwWindows const & windows, dp::ApiVersion api,
                                                           m2::PointU size)
{
  if (api == dp::ApiVersion::Vulkan)
  {
    uint32_t count = 0;
    auto const * extensions = glfwGetRequiredInstanceExtensions(&count);
    CHECK(extensions && count > 0, ("GLFW has no Vulkan surface extensions for the selected platform"));
    // GLFW owns the names until termination, after factory and window destruction.
    auto contextFactory = make_unique_dp<LinuxVulkanContextFactory>(std::span(extensions, count));
    contextFactory->SetSurface(windows.m_visible, size);
    return contextFactory;
  }

  if (api == dp::ApiVersion::OpenGLES3)
    return make_unique_dp<LinuxContextFactory>(windows);

  CHECK(false, ("Unsupported sandbox graphics API", api));
  return nullptr;
}

void OnCreateDrapeEngine(GLFWwindow *, dp::ApiVersion, ref_ptr<dp::GraphicsContextFactory>) {}

void PrepareDestroyContextFactory(ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  if (contextFactory->GetDrawContext()->GetApiVersion() == dp::ApiVersion::Vulkan)
  {
    ref_ptr<LinuxVulkanContextFactory> linuxContextFactory = contextFactory;
    linuxContextFactory->ResetSurface();
  }
}

void UpdateContentScale(GLFWwindow *, float) {}

void UpdateSize(ref_ptr<dp::GraphicsContextFactory>, int, int) {}
