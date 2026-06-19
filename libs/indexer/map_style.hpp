#pragma once

#include <optional>
#include <string>

enum MapStyle
{
  MapStyleDefaultLight = 0,
  MapStyleDefaultDark = 1,
  MapStyleMerged = 2,
  MapStyleVehicleLight = 3,
  MapStyleVehicleDark = 4,
  MapStyleOutdoorsLight = 5,
  MapStyleOutdoorsDark = 6,
  // Add new map style here

  // Specifies number of MapStyle enum values, must be last
  MapStyleCount
};

// A map style "family": the variant selected by the current usage mode, orthogonal to light/dark.
// Extend this enum to introduce a new family.
enum class MapStyleFamily
{
  Default,
  Vehicle,
  Outdoors,
};

extern MapStyle const kDefaultMapStyle;

extern MapStyle MapStyleFromSettings(std::string const & str);
extern std::string MapStyleToString(MapStyle mapStyle);
extern std::string DebugPrint(MapStyle mapStyle);
extern std::string DebugPrint(MapStyleFamily family);
extern bool MapStyleIsDark(MapStyle mapStyle);
extern MapStyle GetDarkMapStyleVariant(MapStyle mapStyle);
extern MapStyle GetLightMapStyleVariant(MapStyle mapStyle);
// Concrete map style for the given family at the given darkness.
extern MapStyle GetMapStyleForFamily(MapStyleFamily family, bool dark);
// True for the Outdoors-family styles.
extern bool IsOutdoorsStyle(MapStyle mapStyle);

// Chooses the live-mode family by priority, highest first: Vehicle while following a vehicle route,
// then Outdoors when the layer flag is set, else Default. Takes pre-resolved booleans so it carries
// no routing/layer dependencies and is directly unit-testable; a higher-priority family (e.g. a
// future Satellite layer) adds an argument and a leading branch.
extern MapStyleFamily SelectMapStyleFamily(bool vehicleFollowing, bool outdoorsEnabled);

// Normalizes a persisted style at startup. There is no active route at launch, so a transient
// navigation (Vehicle) style collapses to the base family — Outdoors when the layer flag is set,
// else Default — at the persisted darkness. Presence-aware: the outdoors flag is authoritative when
// stored; only a legacy Outdoors-only persistence (flag absent) seeds it from the style.
struct StartupMapStyle
{
  MapStyle style;
  bool outdoorsEnabled;
  bool persistOutdoorsFlag;  // true only when the flag was absent and a value was derived to persist
};
extern StartupMapStyle NormalizeStartupMapStyle(MapStyle persisted, std::optional<bool> persistedOutdoorsEnabled);
