// Wraps framework access
#pragma once

#include "map/framework.hpp"

/// Creates framework at first access
Framework & GetFramework();
/// Releases framework resources
void DeleteFramework();
/// Returns true after DeleteFramework() destroyed the singleton (app is terminating).
bool IsFrameworkDestroyed();
