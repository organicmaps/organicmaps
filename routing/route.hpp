#pragma once

#include "base/followed_polyline.hpp"
#include "routing_settings.hpp"
#include "turns.hpp"

#include "geometry/polyline2d.hpp"

#include "std/vector.hpp"
#include "std/set.hpp"
#include "std/string.hpp"


namespace location
{
  class GpsInfo;
  class RouteMatchingInfo;
}

namespace routing
{

class Route
{
public:
  typedef vector<turns::TurnItem> TTurns;
  typedef pair<uint32_t, double> TTimeItem;
  typedef vector<TTimeItem> TTimes;

  explicit Route(string const & router)
    : m_router(router), m_routingSettings(GetCarRoutingSettings()) {}

  template <class TIter>
  Route(string const & router, TIter beg, TIter end)
    : m_router(router), m_routingSettings(GetCarRoutingSettings()), m_poly(beg, end)
  {
    Update();
  }

  Route(string const & router, vector<m2::PointD> const & points, string const & name = string());

  void Swap(Route & rhs);

  template <class TIter> void SetGeometry(TIter beg, TIter end)
  {
    FollowedPolyline(beg, end).Swap(m_poly);
    Update();
  }

  inline void SetTurnInstructions(TTurns & v)
  {
    swap(m_turns, v);
  }

  inline void SetSectionTimes(TTimes & v)
  {
    swap(m_times, v);
  }

  uint32_t GetTotalTimeSec() const;
  uint32_t GetCurrentTimeToEndSec() const;

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly.GetPolyline(); }
  TTurns const & GetTurns() const { return m_turns; }
  void GetTurnsDistances(vector<double> & distances) const;
  string const & GetName() const { return m_name; }
  bool IsValid() const { return (m_poly.GetPolyline().GetSize() > 1); }

  double GetTotalDistanceMeters() const;
  double GetCurrentDistanceFromBeginMeters() const;
  double GetCurrentDistanceToEndMeters() const;
  double GetMercatorDistanceFromBegin() const;

  void GetCurrentTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;
  /// @return true if GetNextTurn() returns a valid result in parameters, false otherwise.
  /// \param distanceToTurnMeters is a distance from current possition to the second turn.
  /// \param turn is information about the second turn.
  /// \note All parameters are filled while a GetNextTurn function call.
  bool GetNextTurn(double & distanceToTurnMeters, turns::TurnItem & turn) const;

  void GetCurrentDirectionPoint(m2::PointD & pt) const;

  /// @return true  If position was updated successfully (projection within gps error radius).
  bool MoveIterator(location::GpsInfo const & info) const;

  /// Square distance to current projection in mercator.
  double GetCurrentSqDistance(m2::PointD const & pt) const;
  void MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const;

  bool IsCurrentOnEnd() const;

  /// Add country name if we have no country filename to make route
  void AddAbsentCountry(string const & name) { m_absentCountries.insert(name); }

  /// Get absent file list of a routing files for shortest path finding
  set<string> const & GetAbsentCountries() const { return m_absentCountries; }

  inline void SetRoutingSettings(RoutingSettings const & routingSettings)
  {
    m_routingSettings = routingSettings;
    Update();
  }

private:
  /// Call this fucnction when geometry have changed.
  void Update();
  double GetPolySegAngle(size_t ind) const;
  TTurns::const_iterator GetCurrentTurn() const;

private:
  friend string DebugPrint(Route const & r);

  string m_router;
  RoutingSettings m_routingSettings;
  string m_name;

  FollowedPolyline m_poly;
  FollowedPolyline m_simplifiedPoly;

  set<string> m_absentCountries;

  TTurns m_turns;
  TTimes m_times;

  mutable double m_currentTime;
};

} // namespace routing
