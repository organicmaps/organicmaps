#pragma once

#include "graphics/color.hpp"
#include "graphics/opengl/route_vertex.hpp"

#include "geometry/polyline2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/vector.hpp"

namespace rg
{

using RV = graphics::gl::RouteVertex;
using TGeometryBuffer = buffer_vector<RV, 128>;
using TIndexBuffer = buffer_vector<unsigned short, 128>;

struct RouteJoinBounds
{
  double m_start = 0;
  double m_end = 0;
  double m_offset = 0;
};

struct RouteData
{
  double m_length;
  TGeometryBuffer m_geometry;
  TIndexBuffer m_indices;
  vector<RouteJoinBounds> m_joinsBounds;
};

class RouteShape
{
public:
  static void PrepareGeometry(m2::PolylineD const & polyline, RouteData & output);
};

} // namespace rg

