#pragma once

#include "com/mapswithme/core/jni_helper.hpp"

#include "partners_api/locals_api.hpp"

#include <vector>

extern jobjectArray ToLocalExpertsArray(std::vector<locals::LocalExpert> const & locals);
