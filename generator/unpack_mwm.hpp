#pragma once

#include "../base/base.hpp"
#include "../std/string.hpp"

// Unpack each section of mwm into a separate file with name filePath.sectionName
void UnpackMwm(string const & filePath);
