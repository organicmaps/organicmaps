// Wraps framework access
#pragma once

#include "../../../map/framework.hpp"


#define MAPSWITHME_PREMIUM_APPSTORE_URL @"itms-apps://itunes.apple.com/app/id510623322"


/// Creates framework at first access
Framework & GetFramework();
/// Releases framework resources
void DeleteFramework();