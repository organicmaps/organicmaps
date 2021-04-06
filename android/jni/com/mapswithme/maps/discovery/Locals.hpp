#pragma once

#include "com/mapswithme/core/jni_helper.hpp"


#include <vector>

extern jobjectArray ToLocalExpertsArray(std::vector<locals::LocalExpert> const & locals);
