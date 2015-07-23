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
  float m_startTexCoord = 0;
  float m_endTexCoord = 1;
  int m_groupIndex = 0;
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
  void Render(shared_ptr<PaintEvent> const & e, ScreenBase const & screen);

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void ConstructRoute(graphics::Screen * dlScreen);
  void ClearRoute(graphics::Screen * dlScreen);
  float CalculateRouteHalfWidth(ScreenBase const & screen, double & zoom) const;
  void CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth, double joinsBoundsScalar);
  void ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength);
  void RenderArrow(graphics::Screen * dlScreen, float halfWidth, ScreenBase const & screen);

  graphics::DisplayList * m_displayList;
  graphics::DisplayList * m_endOfRouteDisplayList;
  graphics::gl::Storage m_storage;

  double m_distanceFromBegin;
  RouteData m_routeData;
  m2::RectF m_arrowTextureRect;
  graphics::Color m_color;
  vector<double> m_turns;
  m2::PointD m_endOfRoutePoint;

  vector<ArrowBorders> m_arrowBorders;
  vector<RouteSegment> m_routeSegments;

  bool m_needClear;
  bool m_waitForConstruction;
};

} // namespace rg

