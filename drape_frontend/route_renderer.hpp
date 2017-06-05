#pragma once

#include "drape_frontend/route_builder.hpp"

#include "drape/drape_global.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace df
{
extern std::string const kRouteColor;
extern std::string const kRouteOutlineColor;
extern std::string const kRoutePedestrian;
extern std::string const kRouteBicycle;

class RouteRenderer final
{
public:
  using TCacheRouteArrowsCallback = std::function<void(dp::DrapeID, std::vector<ArrowBorders> const &)>;

  RouteRenderer();

  void UpdateRoute(ScreenBase const & screen, TCacheRouteArrowsCallback const & callback);

  void RenderRoute(ScreenBase const & screen, bool trafficShown, ref_ptr<dp::GpuProgramManager> mng,
                   dp::UniformValuesStorage const & commonUniforms);

  void AddRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng);
  std::vector<drape_ptr<RouteData>> const & GetRouteData() const;

  void RemoveRouteData(dp::DrapeID segmentId);

  void AddRouteArrowsData(drape_ptr<RouteArrowsData> && routeArrowsData,
                          ref_ptr<dp::GpuProgramManager> mng);

  void Clear();
  void ClearRouteData();
  void ClearGLDependentResources();

  void UpdateDistanceFromBegin(double distanceFromBegin);
  void SetFollowingEnabled(bool enabled);

private:
  struct RouteAdditional
  {
    drape_ptr<RouteArrowsData> m_arrowsData;
    std::vector<ArrowBorders> m_arrowBorders;
    float m_currentHalfWidth = 0.0f;
  };

  void InterpolateByZoom(drape_ptr<RouteSegment> const & routeSegment, ScreenBase const & screen,
                         float & halfWidth, double & zoom) const;
  void RenderRouteData(drape_ptr<RouteData> const & routeData, ScreenBase const & screen,
                       bool trafficShown, ref_ptr<dp::GpuProgramManager> mng,
                       dp::UniformValuesStorage const & commonUniforms);
  void RenderRouteArrowData(dp::DrapeID segmentId, RouteAdditional const & routeAdditional,
                            ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & commonUniforms);

  double m_distanceFromBegin;
  std::vector<drape_ptr<RouteData>> m_routeData;
  std::unordered_map<dp::DrapeID, RouteAdditional> m_routeAdditional;
  bool m_followingEnabled;
};
}  // namespace df
