#pragma once

#include <QIcon>

namespace qt
{
struct Icon
{
  struct Value
  {
    char const * m_path;
    operator QIcon() const { return QIcon(m_path); }
  };

  static constexpr Value Bookmark{":/navig64/bookmark.svg"};
  static constexpr Value BordersSelection{":/navig64/borders_selection.svg"};
  static constexpr Value Bug{":/navig64/bug.svg"};
  static constexpr Value Chart{":/navig64/chart.svg"};
  static constexpr Value CityBoundaries{":/navig64/city_boundaries.svg"};
  static constexpr Value CityRoads{":/navig64/city_roads.svg"};
  static constexpr Value Clear{":/navig64/clear.svg"};
  static constexpr Value ClearRoute{":/navig64/clear-route.svg"};
  static constexpr Value Download{":/navig64/download.svg"};
  static constexpr Value Geom{":/navig64/geom.svg"};
  static constexpr Value Isolines{":/navig64/isolines.svg"};
  static constexpr Value Layers{":/navig64/layers.svg"};
  static constexpr Value Location{":/navig64/location.svg"};
  static constexpr Value LocationSearch{":/navig64/location-search.svg"};
  static constexpr Value PhonePack{":/navig64/phonepack.svg"};
  static constexpr Value PointFinish{":/navig64/point-finish.svg"};
  static constexpr Value PointIntermediate{":/navig64/point-intermediate.svg"};
  static constexpr Value PointStart{":/navig64/point-start.svg"};
  static constexpr Value Routing{":/navig64/routing.svg"};
  static constexpr Value Ruler{":/navig64/ruler.svg"};
  static constexpr Value Run{":/navig64/run.svg"};
  static constexpr Value Search{":/navig64/search.svg"};
  static constexpr Value Select{":/navig64/select.svg"};
  static constexpr Value SelectMode{":/navig64/selectmode.svg"};
  static constexpr Value SettingsRouting{":/navig64/settings-routing.svg"};
  static constexpr Value Subway{":/navig64/subway.svg"};
  static constexpr Value Test{":/navig64/test.svg"};
  static constexpr Value Traffic{":/navig64/traffic.svg"};
};
}  // namespace qt
