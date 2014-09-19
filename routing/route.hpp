#pragma once

#include "../geometry/polyline2d.hpp"
#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"

namespace routing
{

class Route
{
public:
  explicit Route(string const & router, string const & name = string())
    : m_router(router), m_name(name), m_currentSegment(0)
  {
  }

  template <class IterT>
  Route(string const & router, IterT beg, IterT end,
        string const & name = string())
    : m_router(router), m_poly(beg, end), m_name(name), m_currentSegment(0)
  {
    UpdateSegInfo();
  }

  Route(string const & router, vector<m2::PointD> const & points, string const & name = string());

  template <class IterT> void SetGeometry(IterT beg, IterT end)
  {
    m2::PolylineD(beg, end).Swap(m_poly);
    UpdateSegInfo();
  }

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly; }
  string const & GetName() const { return m_name; }

  bool IsValid() const { return m_poly.GetSize() > 0; }

  // Distance of route in meters
  double GetDistance() const;
  double GetDistanceToTarget(m2::PointD const & currPos, double errorRadius, double predictDistance = -1) const;

private:
  double GetDistanceOnPolyline(size_t s1, m2::PointD const & p1, size_t s2, m2::PointD const & p2) const;
  template <class DistanceF> pair<m2::PointD, size_t> GetClosestProjection(m2::PointD const & currPos, double errorRadius, DistanceF const & distFn) const;

protected:
  void UpdateSegInfo();

private:
  friend string DebugPrint(Route const & r);

  /// The source which created the route
  string m_router;
  m2::PolylineD m_poly;
  string m_name;

  vector<double> m_segDistance;
  vector< m2::ProjectionToSection<m2::PointD> > m_segProj;

  mutable size_t m_currentSegment;
  mutable m2::PointD m_currentPoint;

};

} // namespace routing
