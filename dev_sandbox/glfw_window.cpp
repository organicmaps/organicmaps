#include "dev_sandbox/glfw_window.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

// Vulkan types must precede GLFW when this source starts a unity translation unit.
#include <vulkan_wrapper.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GlfwWindow::GlfwWindow(int width, int height) : m_width(width), m_height(height) {}

GlfwWindow::~GlfwWindow()
{
  Destroy();
}

void GlfwWindow::Create(dp::ApiVersion api)
{
  ASSERT_EQUAL(m_mainThread, std::this_thread::get_id(), ());
  CHECK(!m_windows.m_visible && !m_windows.m_upload, ());
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // The first configure must establish the dimensions before graphics initialization.
  glfwWindowHint(GLFW_MAXIMIZED, m_maximized ? GLFW_TRUE : GLFW_FALSE);
#if defined(OMIM_OS_LINUX)
  if (api == dp::ApiVersion::OpenGLES3)
  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  }
#endif
#if defined(OMIM_OS_WINDOWS)
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif
  m_windows.m_visible = glfwCreateWindow(m_width, m_height, "Organic Maps: Developer Sandbox", nullptr, nullptr);
  CHECK(m_windows.m_visible, ("Failed to create sandbox window", api));
  glfwSetWindowUserPointer(m_windows.m_visible, this);
  glfwSetWindowSizeCallback(m_windows.m_visible, [](GLFWwindow * window, int, int)
  {
    auto * owner = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
    owner->RememberGeometry();
  });
#if !defined(OMIM_OS_LINUX)
  glfwSetWindowPosCallback(m_windows.m_visible, [](GLFWwindow * window, int, int)
  {
    auto * owner = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
    owner->RememberGeometry();
  });
#endif
  if (m_position)
    glfwSetWindowPos(m_windows.m_visible, m_position->first, m_position->second);
#if defined(OMIM_OS_LINUX)
  if (api == dp::ApiVersion::OpenGLES3)
  {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    m_windows.m_upload = glfwCreateWindow(1, 1, "Sandbox upload", nullptr, m_windows.m_visible);
    CHECK(m_windows.m_upload, ("Failed to create shared OpenGL upload context"));
  }
#endif
}

void GlfwWindow::PollEvents()
{
  ASSERT_EQUAL(m_mainThread, std::this_thread::get_id(), ());
  CHECK(m_windows.m_visible, ());
  glfwPollEvents();
  bool const maximized = glfwGetWindowAttrib(m_windows.m_visible, GLFW_MAXIMIZED) == GLFW_TRUE;
  if (m_maximized && !maximized)
  {
    // Wayland cannot carry a restored rectangle across replacement windows.
    // Apply it after configure callbacks so they cannot overwrite the requested size.
    glfwSetWindowSize(m_windows.m_visible, m_width, m_height);
#if !defined(OMIM_OS_LINUX)
    if (m_position)
      glfwSetWindowPos(m_windows.m_visible, m_position->first, m_position->second);
#endif
  }
  m_maximized = maximized;
  // Programmatic Wayland resizing can update dimensions without a window-size callback.
  RememberGeometry();
}

void GlfwWindow::RememberGeometry()
{
  if (m_maximized || glfwGetWindowAttrib(m_windows.m_visible, GLFW_MAXIMIZED) ||
      glfwGetWindowAttrib(m_windows.m_visible, GLFW_ICONIFIED))
    return;

  int width, height;
  glfwGetWindowSize(m_windows.m_visible, &width, &height);
  if (width > 0 && height > 0)
  {
    m_width = width;
    m_height = height;
  }
#if !defined(OMIM_OS_LINUX)
  std::pair<int, int> position;
  glfwGetWindowPos(m_windows.m_visible, &position.first, &position.second);
  m_position = position;
#endif
}

void GlfwWindow::Destroy()
{
  ASSERT_EQUAL(m_mainThread, std::this_thread::get_id(), ());
  if (!m_windows.m_visible)
    return;

  RememberGeometry();
  m_maximized = glfwGetWindowAttrib(m_windows.m_visible, GLFW_MAXIMIZED) == GLFW_TRUE;
  if (m_windows.m_upload)
    glfwDestroyWindow(m_windows.m_upload);
  glfwDestroyWindow(m_windows.m_visible);
  m_windows = {};
}
