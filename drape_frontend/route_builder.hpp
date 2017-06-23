#pragma once

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/route_shape.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "traffic/speed_groups.hpp"

#include "geometry/polyline2d.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace df
{
class RouteBuilder
{
public:
  using TFlushRouteFn = function<void(drape_ptr<RouteData> &&)>;
  using TFlushRouteArrowsFn = function<void(drape_ptr<RouteArrowsData> &&)>;

  RouteBuilder(TFlushRouteFn const & flushRouteFn,
               TFlushRouteArrowsFn const & flushRouteArrowsFn);

  void Build(dp::DrapeID subrouteId, drape_ptr<Subroute> && subroute,
             ref_ptr<dp::TextureManager> textures, int recacheId);

  void BuildArrows(dp::DrapeID subrouteId, std::vector<ArrowBorders> const & borders,
                   ref_ptr<dp::TextureManager> textures, int recacheId);

  void ClearRouteCache();

private:
  TFlushRouteFn m_flushRouteFn;
  TFlushRouteArrowsFn m_flushRouteArrowsFn;

  std::unordered_map<dp::DrapeID, m2::PolylineD> m_routeCache;
};
}  // namespace df
