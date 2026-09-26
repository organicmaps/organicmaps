#pragma once

#include <string>

namespace platform
{
// Call before GetPlatform() from the desktop application's main().
void EnableDesktopDataMigration();

// Moves an existing OMaps directory to OrganicMaps for the desktop app.
// Other tools keep using the old path until the app migrates it.
std::string MigrateDesktopDirectory(std::string const & root, bool isDesktopApp);
}  // namespace platform
