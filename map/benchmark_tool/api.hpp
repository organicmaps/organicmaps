#pragma once

#include "../../std/string.hpp"
#include "../../std/utility.hpp"

/// @param[in] count number of times to run benchmark
void RunFeaturesLoadingBenchmark(string const & file, size_t count, pair<int, int> scaleR);
