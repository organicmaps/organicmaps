#pragma once

#include "render/route_shape.hpp"
#include "render/events.hpp"

#include "graphics/color.hpp"
#include "graphics/display_list.hpp"
#include "graphics/defines.hpp"
#include "graphics/screen.hpp"
#include "graphics/opengl/storage.hpp"

#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "platform/location.hpp"

#include "std/vector.hpp"

namespace rg
{

struct ArrowBorders
{
  double m_startDistance = 0;
  double m_endDistance = 0;
  double m_headSize = 0;
  double m_tailSize = 0;

  int m_groupIndex = 0;
  vector<m2::PointD> m_points;

  bool operator==(ArrowBorders const & rhs) const
  {
    double const eps = 1e-7;
    return fabs(m_startDistance - rhs.m_startDistance) < eps &&
           fabs(m_endDistance - rhs.m_endDistance) < eps &&
           fabs(m_headSize - rhs.m_headSize) < eps &&
           fabs(m_tailSize - rhs.m_tailSize) < eps;
  }

  bool operator!=(ArrowBorders const & rhs) const
  {
    return !operator==(rhs);
  }
};

struct RouteSegment
{
  double m_start = 0;
  double m_end = 0;
  bool m_isAvailable = false;

  RouteSegment(double start, double end, bool isAvailable)
    : m_start(start)
    , m_end(end)
    , m_isAvailable(isAvailable)
  {}
};

class RouteRenderer final
{
public:
  RouteRenderer();

  void Setup(m2::PolylineD const & routePolyline, vector<double> const & turns, graphics::Color const & color);
  void Clear();
  void Render(graphics::Screen * dlScreen, ScreenBase const & screen);

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void ConstructRoute(graphics::Screen * dlScreen);
  void ClearRoute(graphics::Screen * dlScreen);
  void InterpolateByZoom(ScreenBase const & screen, float & halfWidth, float & alpha, double & zoom) const;
  void CalculateArrowBorders(m2::RectD const & clipRect, double arrowLength, double scale,
                             double arrowTextureWidth, double joinsBoundsScalar,
                             vector<ArrowBorders> & arrowBorders);
  void ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength,
                        vector<ArrowBorders> & arrowBorders);
  void RenderArrow(graphics::Screen * dlScreen, float halfWidth, ScreenBase const & screen);
  bool RecacheArrows();

  graphics::DisplayList * m_displayList;
  graphics::DisplayList * m_endOfRouteDisplayList;
  graphics::gl::Storage m_storage;

  graphics::DisplayList * m_arrowDisplayList;
  graphics::gl::Storage m_arrowsStorage;

  double m_distanceFromBegin;
  RouteData m_routeData;
  m2::RectF m_arrowTextureRect;
  graphics::Color m_color;
  vector<double> m_turns;
  m2::PointD m_endOfRoutePoint;

  m2::PolylineD m_polyline;
  ArrowsBuffer m_arrowBuffer;

  vector<ArrowBorders> m_arrowBorders;
  vector<RouteSegment> m_routeSegments;

  bool m_needClear;
  bool m_waitForConstruction;
};

} // namespace rg

