#pragma once

#include "graphics/color.hpp"
#include "graphics/opengl/route_vertex.hpp"

#include "geometry/polyline2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/vector.hpp"

namespace rg
{

using TRV = graphics::gl::RouteVertex;
using TGeometryBuffer = buffer_vector<TRV, 128>;
using TIndexBuffer = buffer_vector<unsigned short, 128>;

double const arrowHeightFactor = 96.0 / 36.0;
double const arrowAspect = 400.0 / 192.0;
double const arrowTailSize = 20.0 / 400.0;
double const arrowHeadSize = 124.0 / 400.0;

struct RouteJoinBounds
{
  double m_start = 0;
  double m_end = 0;
  double m_offset = 0;
};

struct RouteData
{
  double m_length;
  vector<pair<TGeometryBuffer, TIndexBuffer>> m_geometry;
  vector<m2::RectD> m_boundingBoxes;
  vector<RouteJoinBounds> m_joinsBounds;

  RouteData() : m_length(0) {}

  void Clear()
  {
    m_geometry.clear();
    m_boundingBoxes.clear();
    m_joinsBounds.clear();
  }
};

struct ArrowsBuffer
{
  TGeometryBuffer m_geometry;
  TIndexBuffer m_indices;
  uint16_t m_indexCounter;

  ArrowsBuffer() : m_indexCounter(0) {}
  void Clear()
  {
    m_geometry.clear();
    m_indices.clear();
    m_indexCounter = 0;
  }
};

class RouteShape
{
public:
  static void PrepareGeometry(m2::PolylineD const & polyline, RouteData & output);
  static void PrepareArrowGeometry(vector<m2::PointD> const & points,
                                   double start, double end, ArrowsBuffer & output);
};

} // namespace rg

