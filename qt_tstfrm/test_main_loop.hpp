#pragma once

#include <functional>

class QPaintDevice;
namespace testing
{
using RenderFunction = std::function<void (QPaintDevice *)>;
using TestFunction = std::function<void (bool apiOpenGLES3)>;
}  // namespace testing

extern void RunTestLoop(char const * testName, testing::RenderFunction && fn, bool autoExit = true);

extern void RunTestInOpenGLOffscreenEnvironment(char const * testName, bool apiOpenGLES3, testing::TestFunction const & fn);
