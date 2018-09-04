#pragma once

#include "routing/road_graph.hpp"
#include "routing/routing_settings.hpp"
#include "routing/segment.hpp"
#include "routing/transit_info.hpp"
#include "routing/turns.hpp"

#include "routing/base/followed_polyline.hpp"

#include "traffic/speed_groups.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/polyline2d.hpp"

#include "base/assert.hpp"

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace location
{
  class GpsInfo;
  class RouteMatchingInfo;
}

namespace routing
{
using SubrouteUid = uint64_t;
SubrouteUid constexpr kInvalidSubrouteId = std::numeric_limits<uint64_t>::max();

/// \brief The route is composed of one or several subroutes. Every subroute is composed of segments.
/// For every Segment is kept some attributes in the structure SegmentInfo.
class RouteSegment final
{
public:
  // Store coefficient where camera placed at the segment (number from 0 to 1)
  // and it's max speed.
  struct SpeedCamera
  {
    SpeedCamera() = default;
    SpeedCamera(double coef, uint8_t maxSpeedKmPH): m_coef(coef), m_maxSpeedKmPH(maxSpeedKmPH) {}

    friend bool operator<(SpeedCamera const & lhs, SpeedCamera const & rhs)
    {
      if (lhs.m_coef != rhs.m_coef)
        return lhs.m_coef < rhs.m_coef;

      return lhs.m_maxSpeedKmPH < rhs.m_maxSpeedKmPH;
    }

    double m_coef = 0.0;
    uint8_t m_maxSpeedKmPH = 0;
  };

  RouteSegment(Segment const & segment, turns::TurnItem const & turn, Junction const & junction,
               std::string const & street, double distFromBeginningMeters,
               double distFromBeginningMerc, double timeFromBeginningS, traffic::SpeedGroup traffic,
               std::unique_ptr<TransitInfo> transitInfo)
    : m_segment(segment)
    , m_turn(turn)
    , m_junction(junction)
    , m_street(street)
    , m_distFromBeginningMeters(distFromBeginningMeters)
    , m_distFromBeginningMerc(distFromBeginningMerc)
    , m_timeFromBeginningS(timeFromBeginningS)
    , m_traffic(traffic)
    , m_transitInfo(move(transitInfo))
  {
  }

  void SetTransitInfo(std::unique_ptr<TransitInfo> transitInfo)
  {
    m_transitInfo.Set(move(transitInfo));
  }

  Segment const & GetSegment() const { return m_segment; }
  Junction const & GetJunction() const { return m_junction; }
  std::string const & GetStreet() const { return m_street; }
  traffic::SpeedGroup GetTraffic() const { return m_traffic; }
  turns::TurnItem const & GetTurn() const { return m_turn; }

  double GetDistFromBeginningMeters() const { return m_distFromBeginningMeters; }
  double GetDistFromBeginningMerc() const { return m_distFromBeginningMerc; }
  double GetTimeFromBeginningSec() const { return m_timeFromBeginningS; }

  bool HasTransitInfo() const { return m_transitInfo.HasTransitInfo(); }
  TransitInfo const & GetTransitInfo() const { return m_transitInfo.Get(); }

  void SetSpeedCameraInfo(std::vector<SpeedCamera> && data) { m_speedCameras = std::move(data); }
  bool IsRealSegment() const { return m_segment.IsRealSegment(); }
  std::vector<SpeedCamera> const & GetSpeedCams() const { return m_speedCameras; }

private:
  Segment m_segment;

  /// Turn (maneuver) information for the turn next to the |m_segment| if any.
  /// If not |m_turn::m_turn| is equal to TurnDirection::NoTurn.
  turns::TurnItem m_turn;

  /// The furthest point of the segment from the beginning of the route along the route.
  Junction m_junction;

  /// Street name of |m_segment| if any. Otherwise |m_street| is empty.
  std::string m_street;

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
};

class Route
{
public:
  using TTurns = std::vector<turns::TurnItem>;
  using TTimeItem = std::pair<uint32_t, double>;
  using TTimes = std::vector<TTimeItem>;
  using TStreetItem = std::pair<uint32_t, std::string>;
  using TStreets = std::vector<TStreetItem>;

  class SubrouteAttrs final
  {
  public:
    SubrouteAttrs() = default;

    SubrouteAttrs(Junction const & start, Junction const & finish, size_t beginSegmentIdx,
                  size_t endSegmentIdx)
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
    {
    }

    Junction const & GetStart() const { return m_start; }
    Junction const & GetFinish() const { return m_finish; }

    size_t GetBeginSegmentIdx() const { return m_beginSegmentIdx; }
    size_t GetEndSegmentIdx() const { return m_endSegmentIdx; }

    size_t GetSize() const { return m_endSegmentIdx - m_beginSegmentIdx; }

  private:

    Junction m_start;
    Junction m_finish;

    // Index of the first subroute segment in the whole route.
    size_t m_beginSegmentIdx = 0;

    // Non inclusive index of the last subroute segment in the whole route.
    size_t m_endSegmentIdx = 0;
  };

  /// \brief For every subroute some attributes are kept in the following structure.
  struct SubrouteSettings final
  {
    SubrouteSettings(RoutingSettings const & routingSettings, std::string const & router,
                     SubrouteUid id)
      : m_routingSettings(routingSettings), m_router(router), m_id(id)
    {
    }

    RoutingSettings const m_routingSettings;
    std::string const m_router;
    /// Some subsystems (for example drape) which is used Route class need to have an id of any subroute.
    /// This subsystems may set the id and then use it. The id is kept in |m_id|.
    SubrouteUid const m_id = kInvalidSubrouteId;
  };

  Route(std::string const & router, uint64_t routeId)
    : m_router(router), m_routingSettings(GetRoutingSettings(VehicleType::Car)), m_routeId(routeId)
  {
  }

  template <class TIter>
  Route(std::string const & router, TIter beg, TIter end, uint64_t routeId)
    : m_router(router)
    , m_routingSettings(GetRoutingSettings(VehicleType::Car))
    , m_poly(beg, end)
    , m_routeId(routeId)
  {
  }

  Route(std::string const & router, std::vector<m2::PointD> const & points, uint64_t routeId,
        std::string const & name = std::string());

  template <class TIter> void SetGeometry(TIter beg, TIter end)
  {
    if (beg == end)
    {
      FollowedPolyline().Swap(m_poly);
    }
    else
    {
      FollowedPolyline(beg, end).Swap(m_poly);
      // If there are no intermediate points it's acceptable to have an empty m_subrouteAttrs.
      // Constructed m_poly will have the last point index as next checkpoint index, it's right.
      if (!m_subrouteAttrs.empty())
      {
        ASSERT_GREATER(m_subrouteAttrs.size(), m_currentSubrouteIdx, ());
        m_poly.SetNextCheckpointIndex(m_subrouteAttrs[m_currentSubrouteIdx].GetEndSegmentIdx());
      }
    }
  }

  template <class SI>
  void SetRouteSegments(SI && v)
  {
    m_routeSegments = std::forward<SI>(v);

    m_haveAltitudes = true;
    for (auto const & s : m_routeSegments)
    {
      if (s.GetJunction().GetAltitude() == feature::kInvalidAltitude)
      {
        m_haveAltitudes = false;
        return;
      }
    }
  }

  std::vector<RouteSegment> & GetRouteSegments() { return m_routeSegments; }
  std::vector<RouteSegment> const & GetRouteSegments() const { return m_routeSegments; }

  void SetCurrentSubrouteIdx(size_t currentSubrouteIdx) { m_currentSubrouteIdx = currentSubrouteIdx; }

  template <class V>
  void SetSubroteAttrs(V && subroutes)
  {
    m_subrouteAttrs = std::forward<V>(subroutes);
    ASSERT_GREATER(m_subrouteAttrs.size(), m_currentSubrouteIdx, ());
    m_poly.SetNextCheckpointIndex(m_subrouteAttrs[m_currentSubrouteIdx].GetEndSegmentIdx());
  }

  void PassNextSubroute()
  {
    ASSERT_GREATER(m_subrouteAttrs.size(), m_currentSubrouteIdx, ());
    m_currentSubrouteIdx = std::min(m_currentSubrouteIdx + 1, m_subrouteAttrs.size() - 1);
    m_poly.SetNextCheckpointIndex(m_subrouteAttrs[m_currentSubrouteIdx].GetEndSegmentIdx());
  }

  /// \returns estimated time for the whole route.
  double GetTotalTimeSec() const;

  /// \returns estimated time to reach the route end.
  double GetCurrentTimeToEndSec() const;

  FollowedPolyline const & GetFollowedPolyline() const { return m_poly; }

  std::string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly.GetPolyline(); }
  
  size_t GetCurrentSubrouteIdx() const { return m_currentSubrouteIdx; }
  std::vector<SubrouteAttrs> const & GetSubroutes() const { return m_subrouteAttrs; }

  std::vector<double> const & GetSegDistanceMeters() const { return m_poly.GetSegDistanceMeters(); }
  bool IsValid() const { return (m_poly.GetPolyline().GetSize() > 1); }

  double GetTotalDistanceMeters() const;
  double GetCurrentDistanceFromBeginMeters() const;
  double GetCurrentDistanceToEndMeters() const;
  double GetMercatorDistanceFromBegin() const;

  /// \brief GetCurrentTurn returns information about the nearest turn.
  /// \param distanceToTurnMeters is a distance from current position to the nearest turn.
  /// \param turn is information about the nearest turn.
  void GetCurrentTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;

  /// \brief Returns a name of a street where the user rides at this moment.
  void GetCurrentStreetName(std::string & name) const;

  /// \brief Returns a name of a street next to idx point of the path. Function avoids short unnamed links.
  void GetStreetNameAfterIdx(uint32_t idx, std::string & name) const;

  /// \brief Gets turn information after the turn next to the nearest one.
  /// \param distanceToTurnMeters is a distance from current position to the second turn.
  /// \param nextTurn is information about the second turn.
  /// \note All parameters are filled while a GetNextTurn function call.
  bool GetNextTurn(double & distanceToTurnMeters, turns::TurnItem & nextTurn) const;
  /// \brief Extract information about zero, one or two nearest turns depending on current position.
  bool GetNextTurns(std::vector<turns::TurnItemDist> & turns) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

  /// @return true  If position was updated successfully (projection within gps error radius).
  bool MoveIterator(location::GpsInfo const & info);

  void MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const;

  /// Add country name if we have no country filename to make route
  void AddAbsentCountry(std::string const & name);

  /// Get absent file list of a routing files for shortest path finding
  std::set<std::string> const & GetAbsentCountries() const { return m_absentCountries; }

  inline void SetRoutingSettings(RoutingSettings const & routingSettings)
  {
    m_routingSettings = routingSettings;
  }

  // Subroute interface.
  /// \returns Number of subroutes.
  /// \note Intermediate points separate a route into several subroutes.
  size_t GetSubrouteCount() const;

  /// \brief Fills |info| with full subroute information.
  /// \param subrouteIdx zero base number of subroute. |segmentIdx| should be less than GetSubrouteCount();
  /// \note |info| is a segment oriented route. Size of |info| is equal to number of points in |m_poly| - 1.
  /// Class Route is a point oriented route. While this conversion some attributes of zero point will be lost.
  /// It happens with zero turn for example.
  /// \note It's a fake implementation for single subroute which is equal to route without any
  /// intermediate points.
  /// Note. SegmentInfo::m_segment is filled with default Segment instance.
  /// Note. SegmentInfo::m_streetName is filled with an empty string.
  void GetSubrouteInfo(size_t subrouteIdx, std::vector<RouteSegment> & segments) const;

  SubrouteAttrs const & GetSubrouteAttrs(size_t subrouteIdx) const;

  /// \returns Subroute settings by |segmentIdx|.
  // @TODO(bykoianko) This method should return SubrouteSettings by reference. Now it returns by value
  // because of fake implementation.
  SubrouteSettings const GetSubrouteSettings(size_t segmentIdx) const;

  bool IsSubroutePassed(size_t subrouteIdx) const;

  /// \brief Sets subroute unique id (|subrouteUid|) by |segmentIdx|.
  /// \note |subrouteUid| is a permanent id of a subroute. This id can be used to address to a subroute
  /// after the route is removed.
  void SetSubrouteUid(size_t segmentIdx, SubrouteUid subrouteUid);

  void GetAltitudes(feature::TAltitudes & altitudes) const;
  bool HaveAltitudes() const { return m_haveAltitudes; }
  traffic::SpeedGroup GetTraffic(size_t segmentIdx) const;

  void GetTurnsForTesting(std::vector<turns::TurnItem> & turns) const;
  bool IsRouteId(uint64_t routeId) const { return routeId == m_routeId; }

private:
  friend std::string DebugPrint(Route const & r);

  double GetPolySegAngle(size_t ind) const;
  void GetClosestTurn(size_t segIdx, turns::TurnItem & turn) const;
  size_t ConvertPointIdxToSegmentIdx(size_t pointIdx) const;

  /// \returns Estimated time to pass the route segment with |segIdx|.
  double GetTimeToPassSegSec(size_t segIdx) const;
  /// \returns Length of the route segment with |segIdx| in meters.
  double GetSegLenMeters(size_t segIdx) const;
  /// \returns ETA to the last passed route point in seconds.
  double GetETAToLastPassedPointSec() const;

  std::string m_router;
  RoutingSettings m_routingSettings;
  std::string m_name;

  FollowedPolyline m_poly;

  std::set<std::string> m_absentCountries;
  std::vector<RouteSegment> m_routeSegments;
  // |m_haveAltitudes| is true if and only if all route points have altitude information.
  bool m_haveAltitudes = false;

  // Subroute
  SubrouteUid m_subrouteUid = kInvalidSubrouteId;
  size_t m_currentSubrouteIdx = 0;
  std::vector<SubrouteAttrs> m_subrouteAttrs;
  // Route identifier. It's unique within single program session.
  uint64_t m_routeId = 0;
};
} // namespace routing
