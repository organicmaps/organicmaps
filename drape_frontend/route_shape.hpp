#pragma once

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/glstate.hpp"
#include "drape/render_bucket.hpp"
#include "drape/utils/vertex_decl.hpp"
#include "drape/pointers.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/polyline2d.hpp"

#include "std/vector.hpp"

namespace df
{

double const kArrowSize = 0.0008;

// Constants below depend on arrow texture.
double const kArrowTextureWidth = 74.0;
double const kArrowTextureHeight = 44.0;
double const kArrowBodyHeight = 24.0;
double const kArrowHeadTextureWidth = 32.0;
double const kArrowTailTextureWidth = 4.0;

double const kArrowHeadSize = kArrowHeadTextureWidth / kArrowTextureWidth;
float const kArrowHeadFactor = 2.0 * kArrowHeadTextureWidth / kArrowTextureHeight;
double const kArrowTailSize = kArrowTailTextureWidth / kArrowTextureWidth;
float const kArrowTailFactor = 2.0 * kArrowTailTextureWidth / kArrowTextureHeight;
double const kArrowHeightFactor = kArrowTextureHeight / kArrowBodyHeight;
double const kArrowAspect = kArrowTextureWidth / kArrowTextureHeight;

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
  m2::PointD m_pivot;
  df::ColorConstant m_color;
  vector<traffic::SpeedGroup> m_traffic;
  double m_length;
  RouteRenderProperty m_route;
  RoutePattern m_pattern;
  int m_recacheId;
};

struct RouteSignData
{
  RouteRenderProperty m_sign;
  bool m_isStart;
  bool m_isValid;
  m2::PointD m_position;
  int m_recacheId;
};

struct RouteArrowsData
{
  RouteRenderProperty m_arrows;
  m2::PointD m_pivot;
  int m_recacheId;
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
  static void PrepareGeometry(vector<m2::PointD> const & path, m2::PointD const & pivot,
                              vector<glsl::vec4> const & segmentsColors,
                              TGeometryBuffer & geometry, TGeometryBuffer & joinsGeometry,
                              double & outputLength);
  static void PrepareArrowGeometry(vector<m2::PointD> const & path, m2::PointD const & pivot,
                                   m2::RectF const & texRect, float depthStep, float depth,
                                   TArrowGeometryBuffer & geometry, TArrowGeometryBuffer & joinsGeometry);
  static void BatchGeometry(dp::GLState const & state, ref_ptr<void> geometry, uint32_t geomSize,
                            ref_ptr<void> joinsGeometry, uint32_t joinsGeomSize,
                            dp::BindingInfo const & bindingInfo, RouteRenderProperty & property);
};

} // namespace df
