#pragma once

class Framework;

namespace sailfish
{
// Owned by OrganicMapsMain(); valid while QML types exist.
Framework & GetFramework();
}  // namespace sailfish
