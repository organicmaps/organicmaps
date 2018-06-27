#pragma once

#include <functional>

class QPaintDevice;
using RenderFunction = std::function<void (QPaintDevice *)>;
using TestFunction = std::function<void (bool apiOpenGLES3)>;

extern void RunTestLoop(char const * testName, RenderFunction && fn, bool autoExit = true);

extern void RunTestInOpenGLOffscreenEnvironment(char const * testName, bool apiOpenGLES3, TestFunction const & fn);
