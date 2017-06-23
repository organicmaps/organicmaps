#pragma once

#include "drape_frontend/circles_pack_shape.hpp"
#include "drape_frontend/route_builder.hpp"

#include "drape/drape_global.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace df
{
extern std::string const kRouteColor;
extern std::string const kRouteOutlineColor;
extern std::string const kRoutePedestrian;
extern std::string const kRouteBicycle;

class RouteRenderer final
{
public:
  using CacheRouteArrowsCallback = std::function<void(dp::DrapeID, std::vector<ArrowBorders> const &)>;
  using PreviewPointsRequestCallback = std::function<void(uint32_t)>;

  struct PreviewInfo
  {
    m2::PointD m_startPoint;
    m2::PointD m_finishPoint;
  };

  RouteRenderer(PreviewPointsRequestCallback && previewPointsRequest);

  void UpdateRoute(ScreenBase const & screen, CacheRouteArrowsCallback const & callback);

  void RenderRoute(ScreenBase const & screen, bool trafficShown, ref_ptr<dp::GpuProgramManager> mng,
                   dp::UniformValuesStorage const & commonUniforms);

  void AddRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng);
  std::vector<drape_ptr<RouteData>> const & GetRouteData() const;

  void RemoveRouteData(dp::DrapeID subrouteId);

  void AddRouteArrowsData(drape_ptr<RouteArrowsData> && routeArrowsData,
                          ref_ptr<dp::GpuProgramManager> mng);

  void AddPreviewRenderData(drape_ptr<CirclesPackRenderData> && renderData,
                            ref_ptr<dp::GpuProgramManager> mng);

  bool UpdatePreview(ScreenBase const & screen);

  void Clear();
  void ClearRouteData();
  void ClearGLDependentResources();

  void UpdateDistanceFromBegin(double distanceFromBegin);
  void SetFollowingEnabled(bool enabled);

  void AddPreviewSegment(dp::DrapeID id, PreviewInfo && info);
  void RemovePreviewSegment(dp::DrapeID id);
  void RemoveAllPreviewSegments();

  void SetSubrouteVisibility(dp::DrapeID id, bool isVisible);

private:
  struct RouteAdditional
  {
    drape_ptr<RouteArrowsData> m_arrowsData;
    std::vector<ArrowBorders> m_arrowBorders;
    float m_currentHalfWidth = 0.0f;
  };

  void RenderRouteData(drape_ptr<RouteData> const & routeData, ScreenBase const & screen,
                       bool trafficShown, ref_ptr<dp::GpuProgramManager> mng,
                       dp::UniformValuesStorage const & commonUniforms);
  void RenderRouteArrowData(dp::DrapeID subrouteId, RouteAdditional const & routeAdditional,
                            ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                            dp::UniformValuesStorage const & commonUniforms);
  void RenderPreviewData(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                         dp::UniformValuesStorage const & commonUniforms);
  void ClearPreviewHandles();
  CirclesPackHandle * GetPreviewHandle(size_t & index);

  double m_distanceFromBegin;
  std::vector<drape_ptr<RouteData>> m_routeData;
  std::unordered_map<dp::DrapeID, RouteAdditional> m_routeAdditional;
  bool m_followingEnabled;
  std::unordered_set<dp::DrapeID> m_hiddenSubroutes;

  PreviewPointsRequestCallback m_previewPointsRequest;
  std::vector<drape_ptr<CirclesPackRenderData>> m_previewRenderData;
  std::vector<std::pair<CirclesPackHandle *, size_t>> m_previewHandlesCache;
  bool m_waitForPreviewRenderData;
  std::unordered_map<dp::DrapeID, PreviewInfo> m_previewSegments;
  m2::PointD m_previewPivot = m2::PointD::Zero();
  std::chrono::steady_clock::time_point m_showPreviewTimestamp;
};
}  // namespace df
