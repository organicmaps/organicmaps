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
    : m_router(router), m_name(name)
  {
  }

  template <class IterT>
  Route(string const & router, IterT beg, IterT end,
        string const & name = string())
    : m_router(router), m_poly(beg, end), m_name(name)
  {
  }

  Route(string const & router, vector<m2::PointD> const & points,
        string const & name = string())
    : m_router(router), m_poly(points), m_name(name)
  {
  }

  template <class IterT> void SetGeometry(IterT beg, IterT end)
  {
    m2::PolylineD(beg, end).Swap(m_poly);
  }

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly; }
  string const & GetName() const { return m_name; }

  bool IsValid() const { return m_poly.GetSize() > 0; }

private:
  friend string DebugPrint(Route const & r);

  /// The source which created the route
  string m_router;
  m2::PolylineD m_poly;
  string m_name;
};

} // namespace routing
