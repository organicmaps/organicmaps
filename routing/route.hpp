#pragma once

#include "../geometry/polyline2d.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"


namespace location { class GpsInfo; }

namespace routing
{

class Route
{
public:
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

  /// Assume that position within road with this tolerance (meters).
  static double GetOnRoadTolerance() { return 20.0; }

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

  void MoveIterator(m2::PointD const & currPos, location::GpsInfo const & info) const;

private:
  /// @param[in]  errorRadius       Radius to check projection candidates (meters).
  /// @param[in]  predictDistance   Predict distance from previous FindProjection call (meters).
  IterT FindProjection(m2::PointD const & currPos,
                       double errorRadius,
                       double predictDistance = -1.0) const;

  template <class DistanceF>
  IterT GetClosestProjection(m2::PointD const & currPos,
                             m2::RectD const & rect,
                             DistanceF const & distFn) const;

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

  /// Cached result iterator for last MoveIterator query.
  mutable IterT m_current;
  mutable double m_currentTime;
};

} // namespace routing
