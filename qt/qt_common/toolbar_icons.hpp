#pragma once

#include <QIcon>

namespace qt
{
enum class ToolbarIcon
{
  Bookmark,
  BordersSelection,
  Bug,
  Chart,
  CityBoundaries,
  CityRoads,
  Clear,
  ClearRoute,
  Download,
  Geom,
  Isolines,
  Layers,
  Location,
  LocationSearch,
  PhonePack,
  PointFinish,
  PointIntermediate,
  PointStart,
  Routing,
  Ruler,
  Run,
  Search,
  Select,
  SelectMode,
  SettingsRouting,
  Subway,
  Test,
  Traffic,
  Up,
  Down,
  Left,
  Right,
};

QIcon GetToolbarIcon(ToolbarIcon icon);
}  // namespace qt
