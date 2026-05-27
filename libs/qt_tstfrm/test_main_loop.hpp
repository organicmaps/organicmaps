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

extern void RunTestInOpenGLOffscreenEnvironment(std::string testName, testing::TestFunction const & fn);
