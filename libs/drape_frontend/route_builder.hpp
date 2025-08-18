#pragma once

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/route_shape.hpp"

#include "drape/pointers.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/polyline2d.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace dp
{
class TextureManager;
class GraphicsContext;
}  // namespace dp

namespace df
{
class RouteBuilder
{
public:
  using FlushFn = std::function<void(drape_ptr<SubrouteData> &&)>;
  using FlushArrowsFn = std::function<void(drape_ptr<SubrouteArrowsData> &&)>;
  using FlushMarkersFn = std::function<void(drape_ptr<SubrouteMarkersData> &&)>;

  RouteBuilder(FlushFn && flushFn, FlushArrowsFn && flushArrowsFn, FlushMarkersFn && flushMarkersFn);

  void Build(ref_ptr<dp::GraphicsContext> context, dp::DrapeID subrouteId, SubrouteConstPtr subroute,
             ref_ptr<dp::TextureManager> textures, int recacheId);

  void BuildArrows(ref_ptr<dp::GraphicsContext> context, dp::DrapeID subrouteId,
                   std::vector<ArrowBorders> const & borders, ref_ptr<dp::TextureManager> textures, int recacheId);

  void ClearRouteCache();

private:
  FlushFn m_flushFn;
  FlushArrowsFn m_flushArrowsFn;
  FlushMarkersFn m_flushMarkersFn;

  struct RouteCacheData
  {
    m2::PolylineD m_polyline;
    double m_baseDepthIndex = 0.0;
  };
  std::unordered_map<dp::DrapeID, RouteCacheData> m_routeCache;
};
}  // namespace df
