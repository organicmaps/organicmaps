#pragma once

#include "routing/routing_settings.hpp"
#include "routing/turns.hpp"

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

  template <class IterT>
  Route(string const & router, IterT beg, IterT end)
    : m_router(router), m_routingSettings(GetCarRoutingSettings()), m_poly(beg, end)
  {
    Update();
  }

  Route(string const & router, vector<m2::PointD> const & points, string const & name = string());

  void Swap(Route & rhs);

  template <class IterT> void SetGeometry(IterT beg, IterT end)
  {
    m2::PolylineD(beg, end).Swap(m_poly);
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

  inline void SetTurnInstructionsGeometry(turns::TTurnsGeom & v)
  {
    swap(m_turnsGeom, v);
  }

  // Time measure are seconds
  uint32_t GetAllTime() const;
  uint32_t GetTime() const;

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly; }
  turns::TTurnsGeom const & GetTurnsGeometry() const { return m_turnsGeom; }
  TTurns const & GetTurns() const { return m_turns; }
  string const & GetName() const { return m_name; }
  bool IsValid() const { return (m_poly.GetSize() > 1); }

  struct IterT
  {
    m2::PointD m_pt;
    size_t m_ind;

    IterT(m2::PointD pt, size_t ind) : m_pt(pt), m_ind(ind) {}
    IterT() : m_ind(-1) {}

    bool IsValid() const { return m_ind != -1; }
  };

  /// @return Distance on route in meters.
  //@{
  double GetDistance() const;
  double GetCurrentDistanceFromBegin() const;
  double GetCurrentDistanceToEnd() const;
  //@}

  void GetTurn(double & distance, turns::TurnItem & turn) const;

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
  }

private:
  /// @param[in]  predictDistance   Predict distance from previous FindProjection call (meters).
  IterT FindProjection(m2::RectD const & posRect, double predictDistance = -1.0) const;

  template <class DistanceF>
  IterT GetClosestProjection(m2::RectD const & posRect, DistanceF const & distFn) const;

  double GetDistanceOnPolyline(IterT const & it1, IterT const & it2) const;

  /// Call this fucnction when geometry have changed.
  void Update();
  double GetPolySegAngle(size_t ind) const;

private:
  friend string DebugPrint(Route const & r);

  string m_router;
  RoutingSettings m_routingSettings;
  m2::PolylineD m_poly;
  string m_name;

  set<string> m_absentCountries;

  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;

  TTurns m_turns;
  TTimes m_times;

  turns::TTurnsGeom m_turnsGeom;

  /// Cached result iterator for last MoveIterator query.
  mutable IterT m_current;
  mutable double m_currentTime;
};

} // namespace routing
