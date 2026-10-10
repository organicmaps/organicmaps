#pragma once

#include "drape/drape_global.hpp"

#include <optional>
#include <thread>
#include <utility>

struct GLFWwindow;

struct GlfwWindows
{
  GLFWwindow * m_visible = nullptr;
  GLFWwindow * m_upload = nullptr;
};

// Windows belong to the main thread and outlive the factory and both Drape threads.
class GlfwWindow
{
public:
  GlfwWindow(int width, int height);
  ~GlfwWindow();
  GlfwWindow(GlfwWindow const &) = delete;
  GlfwWindow & operator=(GlfwWindow const &) = delete;

  void Create(dp::ApiVersion api);
  void Destroy();
  void PollEvents();
  GlfwWindows const & GetWindows() const { return m_windows; }

private:
  void RememberGeometry();

  GlfwWindows m_windows;
  // Normal window dimensions are independent of the maximized framebuffer.
  int m_width;
  int m_height;
  bool m_maximized = true;
  std::optional<std::pair<int, int>> m_position;
  std::thread::id const m_mainThread = std::this_thread::get_id();
};
