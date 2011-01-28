#pragma once

#include "point2d.hpp"

#include "../std/list.hpp"

namespace tesselator
{
  typedef vector<m2::PointD> points_container;
  typedef list<points_container> holes_container;
  void TesselateInterior(points_container const & bound, holes_container const & holes,
                        points_container & triangles);
}
