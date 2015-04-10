#pragma once

#include "std/function.hpp"

class QPaintDevice;
typedef function<void (QPaintDevice *)> TRednerFn;
void RunTestLoop(char const * testName, TRednerFn const & fn, bool autoExit = true);
