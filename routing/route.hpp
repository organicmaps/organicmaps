#pragma once
#include "../geometry/polyline2d.hpp"


namespace routing
{

class Route
{
public:
  m2::PolylineD const & GetPoly() const { return m_poly; }

private:
  m2::PolylineD m_poly;
};

} // namespace routing
