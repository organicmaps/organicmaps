// Wraps framework access
#pragma once

#include "../../../map/framework.hpp"


#define MAPSWITHME_PREMIUM_APPSTORE_URL @"itms://itunes.com/apps/mapswithmepro"


/// Creates framework at first access
Framework & GetFramework();
/// Releases framework resources
void DeleteFramework();