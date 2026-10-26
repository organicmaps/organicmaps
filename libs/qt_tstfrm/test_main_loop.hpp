#pragma once

#include <functional>
#include <string>

class QPaintDevice;
namespace testing
{
using RenderFunction = std::function<void(QPaintDevice *)>;
using TestFunction = std::function<void()>;
}  // namespace testing

extern void RunTestLoop(std::string testName, testing::RenderFunction && fn, bool autoExit = true);

// Requires OpenGL 4.1 core on Windows/macOS and 3.2 core on Linux.
extern void RunTestInOpenGLOffscreenEnvironment(std::string testName, testing::TestFunction const & fn);
