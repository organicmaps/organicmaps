// Wraps framework access
#pragma once

#include "map/framework.hpp"

/// Creates framework at first access
Framework & GetFramework();
/// Releases framework resources
void DeleteFramework();
