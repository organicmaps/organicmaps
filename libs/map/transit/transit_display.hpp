#pragma once

#include "map/transit/transit_reader.hpp"

#include "map/bookmark_manager.hpp"
#include "map/routing_mark.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/route_shape.hpp"

#include "routing/route.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace transit_rendering_tests
{
void UnitTest_TransitDisplay_GateLabelsFollowTraversal();
void UnitTest_TransitDisplay_EntranceIconSurvivesWaypoint();
void UnitTest_TransitDisplay_ExitIconSurvivesWaypoint();
void UnitTest_TransitDisplay_WalkClearsExitIconContext();
void UnitTest_TransitDisplay_WaypointDoesNotDuplicateStopMarks();
void UnitTest_TransitDisplay_DirectionRoundingKeepsTransferAxis();
void UnitTest_TransitDisplay_WalkSeparatesRideStopsAndColors();
void UnitTest_TransitDisplay_UnavailableLegClearsPendingStop();
void UnitTest_TransitDisplay_WaypointTransferHasOneRing();
void UnitTest_TransitDisplay_RemainingTransferKeepsIncomingStyle();
void UnitTest_TransitDisplay_RemainingGateKeepsStopPresentation();
void UnitTest_TransitDisplay_StationaryConnectorPreservesVisualContext();
void UnitTest_TransitDisplay_EmptyWaypointsKeepOneBoundaryRing();
void UnitTest_TransitDisplay_RemainingGroupedBoardingKeepsRenderedDirection();
void UnitTest_TransitDisplay_ExitSeparatesStationaryReboarding();
void UnitTest_TransitDisplay_MissingPreviousMapKeepsAvailableSummary();
void UnitTest_TransitDisplay_NewRouteDoesNotInventPreviousRide();
void UnitTest_TransitDisplay_TransferConnectionKeepsCanonicalRideStart();
}  // namespace transit_rendering_tests

enum class TransitType : uint32_t
{
  // Do not change the order!
  IntermediatePoint,
  Pedestrian,
  Subway,
  Train,
  LightRail,
  Monorail,

  Tram,
  Bus,
  Ferry,
  CableTram,
  AerialLift,
  Funicular,
  Trolleybus,
  AirService,
  WaterService
};

extern std::map<TransitType, std::string> const kTransitSymbols;

std::string DebugPrint(TransitType type);

struct TransitStepInfo
{
  TransitStepInfo() = default;
  TransitStepInfo(TransitType type, double distance, int time, std::string const & number = "", uint32_t color = 0,
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

std::string DebugPrint(TransitStepInfo const & info);

struct TransitRouteInfo
{
  void AddStep(TransitStepInfo const & step, bool allowMerge = true);
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
  TransitTitle(std::string const & text, std::string color) : m_text(text), m_color(std::move(color)) {}

  std::string m_text;
  std::string m_color;
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
  std::string m_color;
  FeatureID m_featureId;
};

struct SubrouteParams
{
  std::string m_lastColor;
  m2::PointD m_lastDir;
  routing::transit::LineId m_lastLineId = routing::transit::kInvalidLineId;
  df::SubrouteMarker m_marker;
  TransitMarkInfo m_transitMarkInfo;
  TransitType m_transitType = TransitType::Pedestrian;
  double m_prevDistance = 0.0;
  int m_prevTime = 0;
};

struct SubrouteSegmentParams
{
  SubrouteSegmentParams(routing::TransitInfo const & transitInfo, TransitDisplayInfo const & displayInfo)
    : m_transitInfo(transitInfo)
    , m_displayInfo(displayInfo)
  {}
  int m_time = 0;
  double m_distance = 0.0;
  double m_startDistanceMerc = 0.0;
  routing::TransitInfo const & m_transitInfo;
  TransitDisplayInfo const & m_displayInfo;
  MwmSet::MwmId m_mwmId;
};

class TransitRouteDisplay
{
public:
  using GetMwmIdFn = std::function<MwmSet::MwmId(routing::NumMwmId numMwmId)>;
  using GetStringsBundleFn = std::function<StringsBundle const &()>;

  TransitRouteDisplay(TransitReadManager & transitReadManager, GetMwmIdFn const & getMwmIdFn,
                      GetStringsBundleFn const & getStringsBundleFn, BookmarkManager * bmManager,
                      std::map<std::string, m2::PointF> const & transitSymbolSizes);

  // Initialize remaining totals and incoming station context at the active subroute.
  // Waypoint indices and cumulative segment distances refer to the original route.
  void SetRouteOrigin(routing::RouteBase const & route);

  // The caller seeds the polyline with the subroute start and sets m_baseDistance to its
  // route-relative Mercator distance. Process subroutes in route order. The final range flushes
  // its alighting ring; other boundary rings wait until the following range classifies the stop.
  bool ProcessSubroute(std::vector<routing::RouteSegment> const & segments, df::Subroute & subroute,
                       bool isLastSubroute);
  void CreateTransitMarks();

  // Empty when transit data for any subroute is unavailable.
  TransitRouteInfo const & GetRouteInfo();

private:
  friend void transit_rendering_tests::UnitTest_TransitDisplay_GateLabelsFollowTraversal();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_EntranceIconSurvivesWaypoint();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_ExitIconSurvivesWaypoint();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_WalkClearsExitIconContext();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_WaypointDoesNotDuplicateStopMarks();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_DirectionRoundingKeepsTransferAxis();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_WalkSeparatesRideStopsAndColors();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_UnavailableLegClearsPendingStop();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_WaypointTransferHasOneRing();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_RemainingTransferKeepsIncomingStyle();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_RemainingGateKeepsStopPresentation();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_StationaryConnectorPreservesVisualContext();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_EmptyWaypointsKeepOneBoundaryRing();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_RemainingGroupedBoardingKeepsRenderedDirection();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_ExitSeparatesStationaryReboarding();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_MissingPreviousMapKeepsAvailableSummary();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_NewRouteDoesNotInventPreviousRide();
  friend void transit_rendering_tests::UnitTest_TransitDisplay_TransferConnectionKeepsCanonicalRideStart();

  using StopId = routing::transit::StopId;
  void FlushPendingStop(df::Subroute & subroute, SubrouteParams & sp);
  void AddEdgeSubwayForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute, SubrouteParams & sp,
                                SubrouteSegmentParams & ssp, StopId legBoardId, StopId legAlightId);

  void AddGateSubwayForSubroute(routing::RouteSegment const & segment, df::Subroute & subroute, SubrouteParams & sp,
                                SubrouteSegmentParams & ssp);

  void CollectTransitDisplayInfo(std::vector<routing::RouteSegment> const & segments,
                                 TransitDisplayInfos & transitDisplayInfos);
  TransitMark * CreateMark(m2::PointD const & pt, FeatureID const & fid);
  void AddStopMark(TransitMarkInfo const & mark);

  TransitReadManager & m_transitReadManager;
  GetMwmIdFn m_getMwmIdFn;
  GetStringsBundleFn m_getStringsBundleFn;
  BookmarkManager * m_bmManager;
  std::map<std::string, m2::PointF> const & m_symbolSizes;

  TransitRouteInfo m_routeInfo;
  std::vector<TransitMarkInfo> m_transitMarks;

  int m_subrouteIndex = 0;
  int m_intermediateIndexOffset = 0;
  double m_distanceOrigin = 0.0;
  int m_timeOrigin = 0;
  float m_maxSubrouteWidth = -1.0f;
  // The entrance and boarding stop can be in different subroutes.
  bool m_pendingEntrance = false;
  // A boundary stop can continue as a boarding stop or precede an exit gate.
  std::optional<size_t> m_pendingStopIndex;
  SubrouteParams m_subrouteParams;
  bool m_transitDataUnavailable = false;
};
