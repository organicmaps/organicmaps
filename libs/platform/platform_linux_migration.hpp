#pragma once

#include <string>

namespace platform
{
// Call before GetPlatform() from the desktop application's main().
void EnableDesktopDataMigration();

// The desktop app moves OMaps to OrganicMaps when possible.
// Other tools use OrganicMaps if it exists, otherwise OMaps.
std::string MigrateDesktopDirectory(std::string const & root, bool isDesktopApp);
}  // namespace platform
