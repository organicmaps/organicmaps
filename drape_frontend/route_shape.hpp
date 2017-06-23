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

#include <vector>

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
float const kArrowHeadFactor = static_cast<float>(2.0 * kArrowHeadTextureWidth / kArrowTextureHeight);
double const kArrowTailSize = kArrowTailTextureWidth / kArrowTextureWidth;
float const kArrowTailFactor = static_cast<float>(2.0 * kArrowTailTextureWidth / kArrowTextureHeight);
double const kArrowHeightFactor = kArrowTextureHeight / kArrowBodyHeight;
double const kArrowAspect = kArrowTextureWidth / kArrowTextureHeight;

enum class RouteType : uint8_t
{
  Car,
  Pedestrian,
  Bicycle,
  Taxi
};

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

struct Subroute
{
  df::RouteType m_routeType;
  m2::PolylineD m_polyline;
  df::ColorConstant m_color;
  std::vector<double> m_turns;
  std::vector<traffic::SpeedGroup> m_traffic;
  double m_baseDistance = 0.0;
  df::RoutePattern m_pattern;

  Subroute() = default;
  Subroute(m2::PolylineD const & polyline, df::ColorConstant color,
           std::vector<double> const & turns,
           std::vector<traffic::SpeedGroup> const & traffic,
           double baseDistance, df::RoutePattern pattern = df::RoutePattern())
    : m_polyline(polyline), m_color(color), m_turns(turns), m_traffic(traffic)
    , m_baseDistance(baseDistance), m_pattern(pattern)
  {}
};

struct RouteRenderProperty
{
  dp::GLState m_state;
  std::vector<drape_ptr<dp::RenderBucket>> m_buckets;
  RouteRenderProperty() : m_state(0, dp::GLState::GeometryLayer) {}
};

struct BaseRouteData
{
  dp::DrapeID m_subrouteId = 0;
  m2::PointD m_pivot = m2::PointD(0.0, 0.0);
  int m_recacheId = -1;
  RouteRenderProperty m_renderProperty;
};

struct RouteData : public BaseRouteData
{
  drape_ptr<Subroute> m_subroute;
  double m_length = 0.0;
};

struct RouteArrowsData : public BaseRouteData {};

struct ArrowBorders
{
  double m_startDistance = 0.0;
  double m_endDistance = 0.0;
  int m_groupIndex = 0;
};

class RouteShape
{
public:
  using RV = gpu::RouteVertex;
  using TGeometryBuffer = buffer_vector<RV, 128>;
  using AV = gpu::SolidTexturingVertex;
  using TArrowGeometryBuffer = buffer_vector<AV, 128>;

  static void CacheRoute(ref_ptr<dp::TextureManager> textures, RouteData & routeData);

  static void CacheRouteArrows(ref_ptr<dp::TextureManager> mng, m2::PolylineD const & polyline,
                               std::vector<ArrowBorders> const & borders,
                               RouteArrowsData & routeArrowsData);

private:
  static void PrepareGeometry(std::vector<m2::PointD> const & path, m2::PointD const & pivot,
                              std::vector<glsl::vec4> const & segmentsColors,
                              TGeometryBuffer & geometry, TGeometryBuffer & joinsGeometry,
                              double & outputLength);
  static void PrepareArrowGeometry(std::vector<m2::PointD> const & path, m2::PointD const & pivot,
                                   m2::RectF const & texRect, float depthStep, float depth,
                                   TArrowGeometryBuffer & geometry,
                                   TArrowGeometryBuffer & joinsGeometry);
  static void BatchGeometry(dp::GLState const & state, ref_ptr<void> geometry, uint32_t geomSize,
                            ref_ptr<void> joinsGeometry, uint32_t joinsGeomSize,
                            dp::BindingInfo const & bindingInfo, RouteRenderProperty & property);
};
}  // namespace df
