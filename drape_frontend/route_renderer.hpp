#pragma once

#include "drape_frontend/route_builder.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <string>

namespace df
{

extern std::string const kRouteColor;
extern std::string const kRouteOutlineColor;
extern std::string const kRoutePedestrian;
extern std::string const kRouteBicycle;

class RouteRenderer final
{
public:
  using TCacheRouteArrowsCallback = function<void(int, vector<ArrowBorders> const &)>;

  RouteRenderer();

  void UpdateRoute(ScreenBase const & screen, TCacheRouteArrowsCallback const & callback);

  void RenderRoute(ScreenBase const & screen, bool trafficShown,
                   ref_ptr<dp::GpuProgramManager> mng,
                   dp::UniformValuesStorage const & commonUniforms);

  void RenderRouteSigns(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                        dp::UniformValuesStorage const & commonUniforms);

  void SetRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng);
  drape_ptr<RouteData> const & GetRouteData() const;

  void SetRouteSign(drape_ptr<RouteSignData> && routeSignData, ref_ptr<dp::GpuProgramManager> mng);
  drape_ptr<RouteSignData> const & GetStartPoint() const;
  drape_ptr<RouteSignData> const & GetFinishPoint() const;

  void SetRouteArrows(drape_ptr<RouteArrowsData> && routeArrowsData, ref_ptr<dp::GpuProgramManager> mng);

  void Clear();
  void ClearGLDependentResources();

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void InterpolateByZoom(ScreenBase const & screen, ColorConstant color,
                         float & halfWidth, double & zoom) const;
  void RenderRouteSign(drape_ptr<RouteSignData> const & sign, ScreenBase const & screen,
                       ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & commonUniforms);

  double m_distanceFromBegin;
  drape_ptr<RouteData> m_routeData;

  vector<ArrowBorders> m_arrowBorders;
  drape_ptr<RouteArrowsData> m_routeArrows;

  drape_ptr<RouteSignData> m_startRouteSign;
  drape_ptr<RouteSignData> m_finishRouteSign;

  float m_currentHalfWidth = 0.0f;
};

} // namespace df
