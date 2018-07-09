#pragma once

#include "com/mapswithme/core/jni_helper.hpp"

#include "partners_api/viator_api.hpp"

#include <vector>

extern jobjectArray ToViatorProductsArray(std::vector<viator::Product> const & products);
