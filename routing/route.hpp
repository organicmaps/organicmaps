#pragma once

#include "routing/turns.hpp"

#include "geometry/polyline2d.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"


namespace location
{
  class GpsInfo;
  class RouteMatchingInfo;
}

namespace routing
{

struct TurnItem
{
  TurnItem()
      : m_index(numeric_limits<uint32_t>::max()),
        m_turn(turns::TurnDirection::NoTurn),
        m_exitNum(0),
        m_keepAnyway(false)
  {
  }

  TurnItem(uint32_t idx, turns::TurnDirection t, uint32_t exitNum = 0)
    : m_index(idx), m_turn(t), m_exitNum(exitNum), m_keepAnyway(false)
  {
  }

  bool operator==(TurnItem const & rhs) const
  {
    return m_index == rhs.m_index && m_turn == rhs.m_turn
        && m_lanes == rhs.m_lanes && m_exitNum == rhs.m_exitNum
        && m_sourceName == rhs.m_sourceName && m_targetName == rhs.m_targetName
        && m_keepAnyway == rhs.m_keepAnyway;
  }

  uint32_t m_index; // Index of point on polyline (number of segment + 1).
  turns::TurnDirection m_turn;
  vector<turns::SingleLaneInfo> m_lanes;  // Lane information on the edge before the turn.
  uint32_t m_exitNum;  // Number of exit on roundabout.
  string m_sourceName;
  string m_targetName;
  // m_keepAnyway is true if the turn shall not be deleted
  // and shall be demonstrated to an end user.
  bool m_keepAnyway;
};

string DebugPrint(TurnItem const & turnItem);

class Route
{
public:
  typedef vector<TurnItem> TTurns;
  typedef pair<uint32_t, double> TTimeItem;
  typedef vector<TTimeItem> TTimes;

  explicit Route(string const & router) : m_router(router) {}

  template <class IterT>
  Route(string const & router, IterT beg, IterT end)
    : m_router(router), m_poly(beg, end)
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

  void SetTurnInstructions(TTurns & v);
  void SetSectionTimes(TTimes & v);
  void SetTurnInstructionsGeometry(turns::TTurnsGeom & v);

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

  void GetTurn(double & distance, TurnItem & turn) const;

  /// @return true  If position was updated successfully (projection within gps error radius).
  bool MoveIterator(location::GpsInfo const & info) const;

  /// Square distance to current projection in mercator.
  double GetCurrentSqDistance(m2::PointD const & pt) const;
  void MatchLocationToRoute(location::GpsInfo & location, location::RouteMatchingInfo & routeMatchingInfo) const;

  bool IsCurrentOnEnd() const;

  /// Add country name if we have no country filename to make route
  void AddAbsentCountry(string const & name) {m_absentCountries.push_back(name);}

  /// Get absent file list of a routing files for shortest path finding
  vector<string> const & GetAbsentCountries() const {return m_absentCountries;}

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
  m2::PolylineD m_poly;
  string m_name;

  vector<string> m_absentCountries;

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
