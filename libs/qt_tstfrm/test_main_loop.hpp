#pragma once

#include <functional>

class QPaintDevice;
namespace testing
{
using RenderFunction = std::function<void(QPaintDevice *)>;
using TestFunction = std::function<void()>;
}  // namespace testing

extern void RunTestLoop(char const * testName, testing::RenderFunction && fn, bool autoExit = true);

extern void RunTestInOpenGLOffscreenEnvironment(char const * testName, testing::TestFunction const & fn);
