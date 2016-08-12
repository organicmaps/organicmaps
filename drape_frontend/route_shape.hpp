#pragma once

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"
#include "drape/utils/vertex_decl.hpp"
#include "drape/pointers.hpp"

#include "geometry/polyline2d.hpp"

#include "std/vector.hpp"

namespace df
{

double const kArrowSize = 0.001;

// Constants below depend on arrow texture.
double const kArrowHeadSize = 124.0 / 400.0;
float const kArrowHeadFactor = 124.0f / 96.0f;

double const kArrowTailSize = 20.0 / 400.0;
float const kArrowTailFactor = 20.0f / 96.0f;

double const kArrowHeightFactor = 96.0 / 36.0;
double const kArrowAspect = 400.0 / 192.0;

struct RoutePattern
{
  bool m_isDashed = false;
  double m_dashLength = 0.0;
  double m_gapLength = 0.0;

  RoutePattern() = default;

  RoutePattern(double dashLength, double gapLength)
    : m_isDashed(true)
    , m_dashLength(dashLength)
    , m_gapLength(gapLength)
  {}
};

struct RouteRenderProperty
{
  dp::GLState m_state;
  vector<drape_ptr<dp::RenderBucket>> m_buckets;
  RouteRenderProperty() : m_state(0, dp::GLState::GeometryLayer) {}
};

struct ArrowBorders
{
  double m_startDistance = 0;
  double m_endDistance = 0;
  int m_groupIndex = 0;
};

struct RouteData
{
  int m_routeIndex;
  m2::PolylineD m_sourcePolyline;
  vector<double> m_sourceTurns;
  df::ColorConstant m_color;
  double m_length;
  RouteRenderProperty m_route;
  RoutePattern m_pattern;
};

struct RouteSignData
{
  RouteRenderProperty m_sign;
  bool m_isStart;
  bool m_isValid;
  m2::PointD m_position;
};

struct RouteArrowsData
{
  RouteRenderProperty m_arrows;
};

class RouteShape
{
public:
  using RV = gpu::RouteVertex;
  using TGeometryBuffer = buffer_vector<RV, 128>;
  using AV = gpu::SolidTexturingVertex;
  using TArrowGeometryBuffer = buffer_vector<AV, 128>;

  static void CacheRoute(ref_ptr<dp::TextureManager> textures, RouteData & routeData);
  static void CacheRouteSign(ref_ptr<dp::TextureManager> mng, RouteSignData & routeSignData);
  static void CacheRouteArrows(ref_ptr<dp::TextureManager> mng, m2::PolylineD const & polyline,
                               vector<ArrowBorders> const & borders, RouteArrowsData & routeArrowsData);

private:
  static void PrepareGeometry(vector<m2::PointD> const & path, TGeometryBuffer & geometry,
                              TGeometryBuffer & joinsGeometry, double & outputLength);
  static void PrepareArrowGeometry(vector<m2::PointD> const & path, m2::RectF const & texRect, float depth,
                                   TArrowGeometryBuffer & geometry, TArrowGeometryBuffer & joinsGeometry);
  static void BatchGeometry(dp::GLState const & state, ref_ptr<void> geometry, size_t geomSize,
                            ref_ptr<void> joinsGeometry, size_t joinsGeomSize,
                            dp::BindingInfo const & bindingInfo, RouteRenderProperty & property);
};

} // namespace df
