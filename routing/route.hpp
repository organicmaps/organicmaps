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
  Route(string const & router, vector<m2::PointD> const & points, string const & name = "");

  string const & GetRouterId() const { return m_router; }
  m2::PolylineD const & GetPoly() const { return m_poly; }
  string const & GetName() const { return m_name; }

  bool IsValid() const { return m_poly.GetSize(); }

private:
  /// The source which created the route
  string m_router;
  m2::PolylineD m_poly;
  string m_name;
};

} // namespace routing
