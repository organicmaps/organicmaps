#pragma once

#include "../geometry/polyline2d.hpp"
#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"

namespace routing
{

class Route
{
public:
  Route(vector<m2::PointD> const & points);

  m2::PolylineD const & GetPoly() const { return m_poly; }

private:
  m2::PolylineD m_poly;
};

} // namespace routing
