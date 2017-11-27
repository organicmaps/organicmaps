#pragma once

#include "map/transit/transit_reader.hpp"

#include "map/bookmark_manager.hpp"
#include "map/routing_mark.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/route_shape.hpp"

#include "routing/route.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

enum class TransitType: uint32_t
{
  // Do not change the order!
  IntermediatePoint,
  Pedestrian,
  Subway,
  Train,
  LightRail,
  Monorail
};

extern std::map<TransitType, std::string> const kTransitSymbols;

struct TransitStepInfo
{
  TransitStepInfo() = default;
  TransitStepInfo(TransitType type, double distance, int time,
                  std::string const & number = "", uint32_t color = 0,
                  int intermediateIndex = 0);

  bool IsEqualType(TransitStepInfo const & ts) const;

  TransitType m_type = TransitType::Pedestrian;

  double m_distanceInMeters = 0.0;
  int m_timeInSec = 0;

  std::string m_distanceStr;
  std::string m_distanceUnitsSuffix;

  // Is valid for all types except TransitType::IntermediatePoint and TransitType::Pedestrian
  std::string m_number;
  uint32_t m_colorARGB = 0;

  // Is valid for TransitType::IntermediatePoint
  int m_intermediateIndex = 0;
};

struct TransitRouteInfo
{
  void AddStep(TransitStepInfo const & step);
  void UpdateDistanceStrings();

  double m_totalDistInMeters = 0.0;
  double m_totalPedestrianDistInMeters = 0.0;
  int m_totalTimeInSec = 0;
  int m_totalPedestrianTimeInSec = 0;

  std::string m_totalDistanceStr;
  std::string m_totalDistanceUnitsSuffix;
  std::string m_totalPedestrianDistanceStr;
  std::string m_totalPedestrianUnitsSuffix;

  std::vector<TransitStepInfo> m_steps;
};

struct TransitTitle
{
  TransitTitle() = default;
  TransitTitle(string const & text, df::ColorConstant const & color) : m_text(text), m_color(color) {}

  string m_text;
  df::ColorConstant m_color;
};

struct TransitMarkInfo
{
  enum class Type
  {
    Stop,
    KeyStop,
    Transfer,
    Gate
  };
  Type m_type = Type::Stop;
  m2::PointD m_point;
  std::vector<TransitTitle> m_titles;
  std::string m_symbolName;
  df::ColorConstant m_color;
  FeatureID m_featureId;
};

class TransitRouteDisplay
{
public:
  using GetMwmIdFn = std::function<MwmSet::MwmId (routing::NumMwmId numMwmId)>;
  using GetStringsBundleFn = std::function<StringsBundle const & ()>;

  TransitRouteDisplay(TransitReadManager & transitReadManager, GetMwmIdFn const & getMwmIdFn,
                      GetStringsBundleFn const & getStringsBundleFn, BookmarkManager * bmManager,
                      std::map<std::string, m2::PointF> const & transitSymbolSizes);

  void ProcessSubroute(std::vector<routing::RouteSegment> const & segments, df::Subroute & subroute);


  TransitRouteInfo const & GetRouteInfo();

private:
  void CollectTransitDisplayInfo(std::vector<routing::RouteSegment> const & segments,
                                 TransitDisplayInfos & transitDisplayInfos);
  void CreateTransitMarks(std::vector<TransitMarkInfo> const & transitMarks);

  TransitReadManager & m_transitReadManager;
  GetMwmIdFn m_getMwmIdFn;
  GetStringsBundleFn m_getStringsBundleFn;
  BookmarkManager * m_bmManager;
  std::map<std::string, m2::PointF> const & m_symbolSizes;

  TransitRouteInfo m_routeInfo;

  int m_subrouteIndex = 0;
};