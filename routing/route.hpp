#pragma once

#include "turns.hpp"

#include "../geometry/polyline2d.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"


namespace location { class GpsInfo; }

namespace routing
{

class Route
{
public:

  struct TurnItem
  {
    TurnItem()
      : m_index(std::numeric_limits<uint32_t>::max())
      , m_turn(turns::NoTurn), m_exitNum(0)
    {
    }

    TurnItem(uint32_t idx, turns::TurnDirection t)
      : m_index(idx), m_turn(t), m_exitNum(0)
    {
    }

    uint32_t m_index; // number of point on polyline (number of segment + 1)
    turns::TurnDirection m_turn;
    uint32_t m_exitNum;  // number of exit on roundabout
    string m_srcName;
    string m_trgName;
  };

  typedef vector<TurnItem> TurnsT;

  typedef pair<uint32_t, double> TimeItemT;
  typedef vector<TimeItemT> TimesT;

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

  void SetTurnInstructions(TurnsT & v);
  void SetSectionTimes(TimesT & v);

  // Time measure are seconds
  uint32_t GetAllTime() const;
  uint32_t GetTime() const;

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly; }
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

  void GetTurn(double & distance, Route::TurnItem & turn) const;

  /// @return true  If position was updated successfully (projection within gps error radius).
  bool MoveIterator(location::GpsInfo const & info) const;

  /// Square distance to current projection in mercator.
  double GetCurrentSqDistance(m2::PointD const & pt) const;
  void MatchLocationToRoute(location::GpsInfo & location) const;

  bool IsCurrentOnEnd() const;

private:
  /// @param[in]  predictDistance   Predict distance from previous FindProjection call (meters).
  IterT FindProjection(m2::RectD const & posRect, double predictDistance = -1.0) const;

  template <class DistanceF>
  IterT GetClosestProjection(m2::RectD const & posRect, DistanceF const & distFn) const;

  double GetDistanceOnPolyline(IterT const & it1, IterT const & it2) const;

  /// Call this fucnction when geometry have changed.
  void Update();

private:
  friend string DebugPrint(Route const & r);

  string m_router;
  m2::PolylineD m_poly;
  string m_name;

  /// Accumulated cache of segments length in meters.
  vector<double> m_segDistance;
  /// Precalculated info for fast projection finding.
  vector<m2::ProjectionToSection<m2::PointD>> m_segProj;

  TurnsT m_turns;
  TimesT m_times;

  /// Cached result iterator for last MoveIterator query.
  mutable IterT m_current;
  mutable double m_currentTime;
};

} // namespace routing
