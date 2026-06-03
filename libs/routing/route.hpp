#pragma once

#include "routing/lanes/lane_info.hpp"
#include "routing/routing_options.hpp"
#include "routing/routing_settings.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"
#include "routing/turns.hpp"

#include "routing/base/followed_polyline.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "traffic/speed_groups.hpp"

#include "platform/country_file.hpp"

#include "geometry/point_with_altitude.hpp"
#include "geometry/polyline2d.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace location
{
class GpsInfo;
class RouteMatchingInfo;
}  // namespace location

namespace routing
{
using SubrouteUid = uint64_t;
SubrouteUid constexpr kInvalidSubrouteId = std::numeric_limits<uint64_t>::max();

using RouteJunctions = std::vector<geometry::PointWithAltitude>;

/// \brief The route is composed of one or several subroutes. Every subroute is composed of segments.
/// For every Segment is kept some attributes in the structure SegmentInfo.
class RouteSegment final
{
public:
  struct SpeedCamera
  {
    SpeedCamera() = default;
    SpeedCamera(double coef, uint8_t maxSpeedKmPH) : m_coef(coef), m_maxSpeedKmPH(maxSpeedKmPH) {}

    bool EqualCoef(SpeedCamera const & rhs) const { return AlmostEqualAbs(m_coef, rhs.m_coef, 1.0E-5); }

    bool operator<(SpeedCamera const & rhs) const
    {
      if (!EqualCoef(rhs))
        return m_coef < rhs.m_coef;

      // Keep a camera with lowest speed (unique).
      return m_maxSpeedKmPH < rhs.m_maxSpeedKmPH;
    }

    friend std::string DebugPrint(SpeedCamera const & rhs);

    /// @todo Can replace with uint16_t feature node index, assuming that all cameras are placed on nodes.
    // Сoefficient where camera placed at the segment (number from 0 to 1).
    double m_coef = 0.0;
    // Max speed
    uint8_t m_maxSpeedKmPH = 0;
  };

  struct RoadNameInfo
  {
    FeatureID m_mwmId;
    // This is for street/road. |m_ref| |m_name|.
    std::string m_name;             // E.g "Johnson Ave.".
    std::string m_destination_ref;  // Number of next road, e.g. "CA 85", Sometimes "CA 85 South". Usually match |m_ref|
                                    // of next main road.
    // This is for 1st segment of link after junction. Exit |junction_ref| to |m_destination_ref| for |m_destination|.
    std::string m_junction_ref;  // Number of junction e.g. "398B".
    std::string m_destination;   // E.g. "Cupertino".
    std::string m_ref;           // Number of street/road e.g. "CA 85".
    bool m_isLink = false;

    RoadNameInfo() = default;
    RoadNameInfo(std::string name) : m_name(std::move(name)) {}
    RoadNameInfo(std::string name, std::string destination_ref, std::string junction_ref)
      : m_name(std::move(name))
      , m_destination_ref(std::move(destination_ref))
      , m_junction_ref(std::move(junction_ref))
    {}

    void ClearUselessStringsForTailSegments()
    {
      // Keep 'name' since it is used in turns comparison.
      m_destination_ref.clear();
      m_junction_ref.clear();
      m_destination.clear();
      m_ref.clear();
    }

    bool HasBasicTextInfo() const { return !m_ref.empty() || !m_name.empty(); }
    bool HasExitInfo() const { return m_isLink || HasExitTextInfo(); }
    bool HasExitTextInfo() const
    {
      return !m_junction_ref.empty() || !m_destination_ref.empty() || !m_destination.empty();
    }
    bool empty() const
    {
      return m_name.empty() && m_ref.empty() && m_junction_ref.empty() && m_destination_ref.empty() &&
             m_destination.empty();
    }

    bool operator==(RoadNameInfo const & rni) const
    {
      return m_name == rni.m_name && m_ref == rni.m_ref && m_junction_ref == rni.m_junction_ref &&
             m_destination_ref == rni.m_destination_ref && m_destination == rni.m_destination;
    }

    friend std::string DebugPrint(RoadNameInfo const & rni);
  };

  RouteSegment(Segment const & segment, turns::TurnItem const & turn, geometry::PointWithAltitude const & junction,
               RoadNameInfo const & roadNameInfo)
    : m_segment(segment)
    , m_turn(turn)
    , m_junction(junction)
    , m_roadNameInfo(roadNameInfo)
    , m_transitInfo(nullptr)
  {}

  void ClearTurn()
  {
    m_turn.m_turn = turns::CarDirection::None;
    m_turn.m_pedestrianTurn = turns::PedestrianDirection::None;
  }

  void SetTurnExits(uint32_t exitNum) { m_turn.m_exitNum = exitNum; }

  turns::lanes::LanesInfo & GetTurnLanes() { return m_turn.m_lanes; }

  void SetDistancesAndTime(double distFromBeginningMeters, double distFromBeginningMerc, double timeFromBeginningS)
  {
    m_distFromBeginningMeters = distFromBeginningMeters;
    m_distFromBeginningMerc = distFromBeginningMerc;
    m_timeFromBeginningS = timeFromBeginningS;
  }

  void SetTransitInfo(std::unique_ptr<TransitInfo> transitInfo) { m_transitInfo.Set(std::move(transitInfo)); }

  Segment const & GetSegment() const { return m_segment; }
  Segment & GetSegment() { return m_segment; }
  geometry::PointWithAltitude const & GetJunction() const { return m_junction; }
  RoadNameInfo const & GetRoadNameInfo() const { return m_roadNameInfo; }
  turns::TurnItem const & GetTurn() const { return m_turn; }
  void ClearTurnLanes() { m_turn.m_lanes.clear(); }

  double GetDistFromBeginningMeters() const { return m_distFromBeginningMeters; }
  double GetDistFromBeginningMerc() const { return m_distFromBeginningMerc; }
  double GetTimeFromBeginningSec() const { return m_timeFromBeginningS; }

  bool HasTransitInfo() const { return m_transitInfo.HasTransitInfo(); }
  TransitInfo const & GetTransitInfo() const { return m_transitInfo.Get(); }

  void SetSpeedCameraInfo(std::vector<SpeedCamera> && data) { m_speedCameras = std::move(data); }
  std::vector<SpeedCamera> const & GetSpeedCams() const { return m_speedCameras; }

  RoutingOptions GetRoadTypes() const { return m_roadTypes; }
  void SetRoadTypes(RoutingOptions types) { m_roadTypes = types; }

  traffic::SpeedGroup GetTraffic() const { return m_traffic; }
  void SetTraffic(traffic::SpeedGroup group) { m_traffic = group; }

  SpeedInUnits GetSpeedLimit(time_t time) const
  {
    ASSERT(time > 0, ());
    return m_speedLimit.GetCurrentSpeed(time, m_segment.IsForward());
  }
  void SetSpeedLimit(Maxspeed speed) { m_speedLimit = std::move(speed); }

  /// Is 'from' a segregated turn before this turn.
  bool IsSegregatedTurn(RouteSegment const & from) const;
  /// Merge lanes info from segregated 'from' into this if needed.
  void MergeLanes(RouteSegment & from);

private:
  Segment m_segment;

  /// Turn (maneuver) information for the turn next to the |m_segment| if any.
  /// |m_turn::m_index| == segment index + 1.
  /// If not |m_turn::m_turn| is equal to TurnDirection::None.
  turns::TurnItem m_turn;

  /// The furthest point of the segment from the beginning of the route along the route.
  geometry::PointWithAltitude m_junction;

  /// RoadNameInfo of |m_segment| if any. Otherwise |m_roadNameInfo| is empty.
  RoadNameInfo m_roadNameInfo;

  /// Speed limit of |m_segment| if any.
  Maxspeed m_speedLimit;

  /// Distance from the route (not the subroute) beginning to the farthest end of |m_segment| in meters.
  double m_distFromBeginningMeters = 0.0;

  /// Distance from the route (not the subroute) beginning to the farthest end of |m_segment| in mercator.
  double m_distFromBeginningMerc = 0.0;

  /// ETA from the route beginning (not the subroute) in seconds to reach the farthest from the route beginning
  /// end of |m_segment|.
  double m_timeFromBeginningS = 0.0;

  traffic::SpeedGroup m_traffic = traffic::SpeedGroup::Unknown;

  /// Information needed to display transit segments properly.
  TransitInfoWrapper m_transitInfo;

  // List of speed cameras, sorted by segment direction (from start to end).
  // Stored coefficients where they placed at the segment (numbers from 0 to 1)
  // and theirs' max speed.
  std::vector<SpeedCamera> m_speedCameras;

  RoutingOptions m_roadTypes;
};

/// \brief Holds route geometry, segments, and metadata needed to display a route on the map.
/// Does not carry any follow-time state (current position, projection cache, etc.) — that lives on Route.
/// Alternative (non-followed) routes are represented as RouteBase instances inside RoutesResult.
class RouteBase
{
public:
  class SubrouteAttrs final
  {
  public:
    SubrouteAttrs() = default;

    SubrouteAttrs(geometry::PointWithAltitude const & start, geometry::PointWithAltitude const & finish,
                  size_t beginSegmentIdx, size_t endSegmentIdx)
      : m_start(start)
      , m_finish(finish)
      , m_beginSegmentIdx(beginSegmentIdx)
      , m_endSegmentIdx(endSegmentIdx)
    {
      CHECK_LESS_OR_EQUAL(beginSegmentIdx, endSegmentIdx, ());
    }

    SubrouteAttrs(SubrouteAttrs const & subroute, size_t beginSegmentIdx)
      : m_start(subroute.m_start)
      , m_finish(subroute.m_finish)
      , m_beginSegmentIdx(beginSegmentIdx)
      , m_endSegmentIdx(beginSegmentIdx + subroute.GetSize())
    {}

    geometry::PointWithAltitude const & GetStart() const { return m_start; }
    geometry::PointWithAltitude const & GetFinish() const { return m_finish; }

    size_t GetBeginSegmentIdx() const { return m_beginSegmentIdx; }
    size_t GetEndSegmentIdx() const { return m_endSegmentIdx; }

    size_t GetSize() const { return m_endSegmentIdx - m_beginSegmentIdx; }

  private:
    geometry::PointWithAltitude m_start;
    geometry::PointWithAltitude m_finish;

    // Index of the first subroute segment in the whole route.
    size_t m_beginSegmentIdx = 0;

    // Non inclusive index of the last subroute segment in the whole route.
    size_t m_endSegmentIdx = 0;
  };

  RouteBase() = default;

  std::vector<RouteSegment> & GetRouteSegments() { return m_routeSegments; }
  std::vector<RouteSegment> const & GetRouteSegments() const { return m_routeSegments; }

  void SetCurrentSubrouteIdx(size_t currentIdx)
  {
    ASSERT_LESS(currentIdx, m_subrouteAttrs.size(), ());
    m_currentSubrouteIdx = currentIdx;
  }

  size_t GetCurrentSubrouteIdx() const { return m_currentSubrouteIdx; }
  std::vector<SubrouteAttrs> const & GetSubroutes() const { return m_subrouteAttrs; }

  // A route is "valid" (for display purposes) when it has at least one segment and a starting subroute.
  bool IsValid() const { return !m_routeSegments.empty() && !m_subrouteAttrs.empty(); }

  /// \returns The midpoint of the longest divergence span — used to place the ETA balloon
  /// where the alt actually differs from |origin| — or nullopt if the route fails the threshold (equal).
  std::optional<m2::PointD> FindMaxDiffMidpoint(std::vector<RouteSegment> const & origin) const;

  /// \brief Polyline midpoint of segments [beginIdx, endIdx] interpolated to half their geodesic
  /// length. The 0-based indexing is into m_routeSegments; the previous "junction" of segment 0
  /// is the first subroute's start point. Both bounds are inclusive.
  m2::PointD GetMidpoint(size_t beginIdx, size_t endIdx) const;
  /// \returns Midpoint of the whole route, or PointD{} when the route has no segments.
  m2::PointD GetMidpoint() const
  {
    ASSERT(IsValid(), ());
    return GetMidpoint(0, m_routeSegments.size() - 1);
  }

  /// \returns Pivot point for the alt's ETA balloon, set by FindMaxDiffMidpoint on the alt side
  /// and used by RoutingManager::CreateRouteAltMarks. Unset on the active route.
  std::optional<m2::PointD> const & GetDiffMidpoint() const { return m_diffMidpoint; }
  void SetDiffMidpoint(m2::PointD const & pt) { m_diffMidpoint = pt; }

  double GetTotalTimeSec() const;

  /// \brief Mercator bounding rect of the route's points, derived from segments + first subroute start.
  m2::RectD GetLimitRect() const;

  // Subroute interface.
  /// \returns Number of subroutes.
  /// \note Intermediate points separate a route into several subroutes.
  size_t GetSubrouteCount() const;

  /// \brief Fills |segments| with full subroute information.
  void GetSubrouteInfo(size_t subrouteIdx, std::vector<RouteSegment> & segments) const;

  SubrouteAttrs const & GetSubrouteAttrs(size_t subrouteIdx) const;

  void GetAltitudes(geometry::Altitudes & altitudes) const;
  bool HaveAltitudes() const { return m_haveAltitudes; }
  traffic::SpeedGroup GetTraffic(size_t segmentIdx) const;

  void GetTurnsForTesting(std::vector<turns::TurnItem> & turns) const;

  /// \returns Length of the route segment with |segIdx| in meters.
  double GetSegLenMeters(size_t segIdx) const;

  void SetMwmsPartlyProhibitedForSpeedCams(std::vector<platform::CountryFile> && mwms);

  /// \returns true if the route crosses at least one mwm where there are restrictions on warning
  /// about speed cameras.
  bool CrossMwmsPartlyProhibitedForSpeedCams() const;

  /// \returns mwm list which is crossed by the route and where there are restrictions on warning
  /// about speed cameras.
  std::vector<platform::CountryFile> const & GetMwmsPartlyProhibitedForSpeedCams() const;

  template <class FnT>
  void ForEachPoint(FnT && fn) const
  {
    if (!m_subrouteAttrs.empty())
    {
      fn(m_subrouteAttrs.front().GetStart());
      for (auto const & s : m_routeSegments)
        fn(s.GetJunction());
    }
  }

protected:
  void SetRouteSegments(std::vector<RouteSegment> && routeSegments);

  std::vector<RouteSegment> m_routeSegments;
  // |m_haveAltitudes| is true if and only if all route points have altitude information.
  bool m_haveAltitudes = false;

  // Subroute
  size_t m_currentSubrouteIdx = 0;
  std::vector<SubrouteAttrs> m_subrouteAttrs;

  // Mwms which are crossed by the route where speed cameras are prohibited.
  std::vector<platform::CountryFile> m_speedCamPartlyProhibitedMwms;

  // Pivot for the alternative-route ETA balloon — midpoint of the longest segment span.
  std::optional<m2::PointD> m_diffMidpoint;
};

/// \brief A RouteBase that the user is actively following: adds the matched position on the polyline,
/// precomputed projection cache, and queries that depend on the current position
/// (turn distances, time-to-end, current street, etc.).
class Route : public RouteBase
{
public:
  using SubrouteAttrs = RouteBase::SubrouteAttrs;

  Route() = default;

  /// \brief Promote an alternative (RouteBase) to a followed Route. The base's segments + first subroute
  /// start are used to (re)build the FollowedPolyline for follow-time matching.
  explicit Route(RouteBase const & base) : RouteBase(base) { RebuildFollowedPolyline(); }

  Route(Route const & rhs) = default;

  template <class TIter>
  void SetGeometry(TIter beg, TIter end)
  {
    if (beg == end)
      FollowedPolyline().Swap(m_poly);
    else
    {
      FollowedPolyline(beg, end).Swap(m_poly);
      if (!m_subrouteAttrs.empty())
        UpdatePolySubrouteIdx();
    }
  }

  // Hide base's setters so the FollowedPolyline stays in sync with route structure.
  void SetRouteSegments(std::vector<RouteSegment> && routeSegments);

  template <class V>
  void SetSubroutes(V && subroutes, size_t currentIdx = 0)
  {
    m_subrouteAttrs = std::forward<V>(subroutes);
    RouteBase::SetCurrentSubrouteIdx(currentIdx);
    UpdatePolySubrouteIdx();
  }

  void PassNextSubroute()
  {
    ASSERT_LESS(m_currentSubrouteIdx, m_subrouteAttrs.size(), ());
    m_currentSubrouteIdx = std::min(m_currentSubrouteIdx + 1, m_subrouteAttrs.size() - 1);
    UpdatePolySubrouteIdx();
  }

  /// \returns estimated time to reach the route end.
  double GetCurrentTimeToEndSec() const;

  /// \brief estimated time to reach segment.
  double GetCurrentTimeToSegmentSec(size_t segIdx) const;

  /// \brief estimated time to the nearest turn.
  double GetCurrentTimeToNearestTurnSec() const;

  // A Route is "valid for following" when the followed polyline has >= 2 points and a valid iterator.
  bool IsValid() const { return m_poly.IsValid(); }

  // Used in tests only.
  FollowedPolyline && MoveFollowedPolyline() { return std::move(m_poly); }
  m2::PolylineD const & GetPoly() const { return m_poly.GetPolyline(); }

  /// \brief Current matched position iterator on the polyline.
  FollowedPolyline::Iter GetCurrentIter() const { return m_poly.GetCurrentIter(); }

  /// \brief Total geodesic length of the followed polyline.
  double GetTotalDistanceMeters() const { return m_poly.IsValid() ? m_poly.GetTotalDistanceMeters() : 0.0; }
  /// \brief Cumulative geodesic distance to the end of each polyline segment.
  std::vector<double> const & GetSegDistanceMeters() const { return m_poly.GetSegDistanceMeters(); }

  void SetRoutingSettings(RoutingSettings const & routingSettings) { m_routingSettings = routingSettings; }

  double GetCurrentDistanceFromBeginMeters() const;
  double GetCurrentDistanceToEndMeters() const;
  double GetMercatorDistanceFromBegin() const;

  /// \brief Extracts information about the nearest turn from the remaining part of the route.
  /// \param distanceToTurnMeters is a distance from current position to the nearest turn.
  /// \param turn is information about the nearest turn.
  void GetNearestTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;

  /// \returns Information about turn from RouteSegment according to current iterator set with MoveIterator() method.
  turns::TurnItem GetCurrentIteratorTurn() const;

  /// \brief Returns first non-empty name info of a street starting from segIdx.
  void GetClosestStreetNameAfterIdx(size_t segIdx, RouteSegment::RoadNameInfo & roadNameInfo) const;

  /// \brief Returns name info of a street where the user rides at this moment.
  void GetCurrentStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const;

  /// \brief Returns current speed limit
  SpeedInUnits GetCurrentSpeedLimit() const;

  /// \brief Return name info of a street according to the next turn.
  void GetNextTurnStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const;

  /// \brief Return name info of a street according to the next next turn.
  void GetNextNextTurnStreetName(RouteSegment::RoadNameInfo & roadNameInfo) const;

  /// \brief Gets turn information after the turn next to the nearest one.
  bool GetNextTurn(double & distanceToTurnMeters, turns::TurnItem & nextTurn) const;
  /// \brief Extract information about zero, one or two nearest turns depending on current position.
  bool GetNextTurns(std::vector<turns::TurnItemDist> & turns) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

  bool MoveIterator(location::GpsInfo const & info);

  /// \brief Finds projection of |location| to the nearest route and sets |routeMatchingInfo|.
  /// fields accordingly.
  bool MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const;

  bool IsSubroutePassed(size_t subrouteIdx) const;

  std::string DebugPrintTurns() const;

private:
  friend std::string DebugPrint(Route const & r);

  double GetPolySegAngle(size_t ind) const;
  void GetClosestTurnAfterIdx(size_t segIdx, turns::TurnItem & turn) const;

  /// \returns Estimated time from the beginning.
  double GetCurrentTimeFromBeginSec() const;

  /// \brief Rebuild the FollowedPolyline from RouteBase segments + first subroute start.
  /// Used by the Route(RouteBase) promote constructors.
  void RebuildFollowedPolyline();

  void UpdatePolySubrouteIdx()
  {
    ASSERT_LESS(m_currentSubrouteIdx, m_subrouteAttrs.size(), ());
    m_poly.SetNextCheckpointIndex(m_subrouteAttrs[m_currentSubrouteIdx].GetEndSegmentIdx());
  }

  void UpdatePolyFakeIdx();

  // The followed polyline (geometry + matched-position state + projection cache + fake-segment marks).
  // For "fresh" Routes built via SetGeometry/SetRouteSegments the polyline is set directly;
  // for promoted alternatives, it is reconstructed from base segments by RebuildFollowedPolyline().
  FollowedPolyline m_poly;

  // Vehicle-specific tuning (matching threshold, finish tolerance, etc.). Set by RoutingSession after
  // promoting an alternative to a followed Route.
  RoutingSettings m_routingSettings = GetRoutingSettings(VehicleType::Car);
};

/// \brief Result of a route calculation. Carries one or more alternative routes (each as a RouteBase),
/// plus identification info shared by the request as a whole. The "active" alternative is the one to follow.
class RoutesResult
{
public:
  RoutesResult() = default;
  RoutesResult(std::string routerName, uint64_t routesId) : m_routerName(std::move(routerName)), m_routesId(routesId) {}

  void MakeFrom(std::string name, Route && route)
  {
    m_routerName = std::move(name);
    m_routes.clear();
    m_routes.emplace_back(std::move(static_cast<RouteBase &>(route)));
    m_activeIdx = 0;
  }

  bool IsValid() const { return !m_routes.empty() && m_routes[m_activeIdx].IsValid(); }

  RouteBase & GetActive()
  {
    ASSERT_LESS(m_activeIdx, m_routes.size(), ());
    return m_routes[m_activeIdx];
  }

  RouteBase const & GetActive() const
  {
    ASSERT_LESS(m_activeIdx, m_routes.size(), ());
    return m_routes[m_activeIdx];
  }

  std::vector<RouteBase> m_routes;
  size_t m_activeIdx = 0;
  std::string m_routerName;
  // Session-unique id, shared by all alternatives in this result.
  uint64_t m_routesId = 0;
};

/// \returns true if |turn| is not equal to turns::CarDirection::None or
/// |turns::PedestrianDirection::None|.
bool IsNormalTurn(turns::TurnItem const & turn);
}  // namespace routing
