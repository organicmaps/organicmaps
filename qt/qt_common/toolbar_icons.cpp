#include "qt/qt_common/toolbar_icons.hpp"

#include "qt/qt_common/helpers.hpp"

namespace qt
{
namespace
{
char const * LightIconPath(ToolbarIcon icon)
{
  switch (icon)
  {
  case ToolbarIcon::Bookmark: return (":/navig64-light/bookmark.png");
  case ToolbarIcon::BordersSelection: return (":/navig64-light/borders_selection.png");
  case ToolbarIcon::Bug: return (":/navig64-light/bug.png");
  case ToolbarIcon::Chart: return (":/navig64-light/chart.png");
  case ToolbarIcon::CityBoundaries: return (":/navig64-light/city_boundaries.png");
  case ToolbarIcon::CityRoads: return (":/navig64-light/city_roads.png");
  case ToolbarIcon::Clear: return (":/navig64-light/clear.png");
  case ToolbarIcon::ClearRoute: return (":/navig64-light/clear-route.png");
  case ToolbarIcon::Download: return (":/navig64-light/download.png");
  case ToolbarIcon::Geom: return (":/navig64-light/geom.png");
  case ToolbarIcon::Isolines: return (":/navig64-light/isolines.png");
  case ToolbarIcon::Layers: return (":/navig64-light/layers.png");
  case ToolbarIcon::Location: return (":/navig64-light/location.png");
  case ToolbarIcon::LocationSearch: return (":/navig64-light/location-search.png");
  case ToolbarIcon::PhonePack: return (":/navig64-light/phonepack.png");
  case ToolbarIcon::PointFinish: return (":/navig64-light/point-finish.png");
  case ToolbarIcon::PointIntermediate: return (":/navig64-light/point-intermediate.png");
  case ToolbarIcon::PointStart: return (":/navig64-light/point-start.png");
  case ToolbarIcon::Routing: return (":/navig64-light/routing.png");
  case ToolbarIcon::Ruler: return (":/navig64-light/ruler.png");
  case ToolbarIcon::Run: return (":/navig64-light/run.png");
  case ToolbarIcon::Search: return (":/navig64-light/search.png");
  case ToolbarIcon::Select: return (":/navig64-light/select.png");
  case ToolbarIcon::SelectMode: return (":/navig64-light/selectmode.png");
  case ToolbarIcon::SettingsRouting: return (":/navig64-light/settings-routing.png");
  case ToolbarIcon::Subway: return (":/navig64-light/subway.png");
  case ToolbarIcon::Test: return (":/navig64-light/test.png");
  case ToolbarIcon::Traffic: return (":/navig64-light/traffic.png");
  case ToolbarIcon::Up: return (":/navig64-light/up.png");
  case ToolbarIcon::Down: return (":/navig64-light/down.png");
  case ToolbarIcon::Left: return (":/navig64-light/left.png");
  case ToolbarIcon::Right: return (":/navig64-light/right.png");
  }
  return (":/navig64-light/search.png");
}
}  // namespace

QIcon GetToolbarIcon(ToolbarIcon icon)
{
  return common::GetToolbarIcon(LightIconPath(icon));
}
}  // namespace qt
