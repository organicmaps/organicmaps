#pragma once

#include "drape_frontend/route_builder.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

namespace df
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

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  void SetRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng);

  void Clear();

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void CalculateArrowBorders(drape_ptr<ArrowRenderProperty> const & property, double arrowLength,
                             double scale, double arrowTextureWidth, double joinsBoundsScalar);
  void ApplyJoinsBounds(drape_ptr<ArrowRenderProperty> const & property, double joinsBoundsScalar,
                        double glbHeadLength, vector<ArrowBorders> & arrowBorders);
  void RenderArrow(ref_ptr<dp::GpuProgram> prg, drape_ptr<ArrowRenderProperty> const & property,
                   float halfWidth, ScreenBase const & screen);
  void InterpolateByZoom(ScreenBase const & screen, float & halfWidth, float & alpha, double & zoom) const;

  double m_distanceFromBegin;
  drape_ptr<RouteData> m_routeData;

  vector<ArrowBorders> m_arrowBorders;
  vector<RouteSegment> m_routeSegments;
};

} // namespace df
