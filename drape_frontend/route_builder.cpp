#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace
{
std::vector<drape_ptr<df::SubrouteData>> SplitSubroute(dp::DrapeID subrouteId,
                                                       df::SubrouteConstPtr subroute,
                                                       int recacheId)
{
  ASSERT(subroute != nullptr, ());

  std::vector<drape_ptr<df::SubrouteData>> result;
  if (subroute->m_styleType == df::SubrouteStyleType::Single)
  {
    ASSERT(!subroute->m_style.empty(), ());
    auto subrouteData = make_unique_dp<df::SubrouteData>();
    subrouteData->m_subrouteId = subrouteId;
    subrouteData->m_subroute = subroute;
    subrouteData->m_pivot = subrouteData->m_subroute->m_polyline.GetLimitRect().Center();
    subrouteData->m_recacheId = recacheId;
    subrouteData->m_styleIndex = 0;
    subrouteData->m_startPointIndex = 0;
    subrouteData->m_endPointIndex = subrouteData->m_subroute->m_polyline.GetSize() - 1;
    result.push_back(std::move(subrouteData));
    return result;
  }

  ASSERT_EQUAL(subroute->m_style.size() + 1, subroute->m_polyline.GetSize(), ());

  size_t startIndex = 0;
  result.reserve(10);
  for (size_t i = 1; i <= subroute->m_style.size(); ++i)
  {
    if (i == subroute->m_style.size() || subroute->m_style[i] != subroute->m_style[startIndex])
    {
      auto subrouteData = make_unique_dp<df::SubrouteData>();
      subrouteData->m_subrouteId = subrouteId;
      subrouteData->m_subroute = subroute;
      subrouteData->m_startPointIndex = startIndex;
      subrouteData->m_endPointIndex = i;
      subrouteData->m_styleIndex = startIndex;
      subrouteData->m_pivot = subrouteData->m_subroute->m_polyline.GetLimitRect().Center();
      subrouteData->m_recacheId = recacheId;
      result.push_back(std::move(subrouteData));

      startIndex = i;
    }
  }
  return result;
}
}  // namespace

namespace df
{
RouteBuilder::RouteBuilder(TFlushRouteFn const & flushRouteFn,
                           TFlushRouteArrowsFn const & flushRouteArrowsFn)
  : m_flushRouteFn(flushRouteFn)
  , m_flushRouteArrowsFn(flushRouteArrowsFn)
{}

void RouteBuilder::Build(dp::DrapeID subrouteId, SubrouteConstPtr subroute,
                         ref_ptr<dp::TextureManager> textures, int recacheId)
{
  RouteCacheData cacheData;
  cacheData.m_polyline = subroute->m_polyline;
  cacheData.m_baseDepthIndex = subroute->m_baseDepthIndex;
  m_routeCache[subrouteId] = std::move(cacheData);

  auto subrouteData = SplitSubroute(subrouteId, subroute, recacheId);
  for (auto & data : subrouteData)
    RouteShape::CacheRoute(textures, *data.get());

  // Flush route geometry.
  GLFunctions::glFlush();

  if (m_flushRouteFn != nullptr)
  {
    for (auto & data : subrouteData)
      m_flushRouteFn(std::move(data));
    subrouteData.clear();
  }
}

void RouteBuilder::ClearRouteCache()
{
  m_routeCache.clear();
}

void RouteBuilder::BuildArrows(dp::DrapeID subrouteId, std::vector<ArrowBorders> const & borders,
                               ref_ptr<dp::TextureManager> textures, int recacheId)
{
  auto it = m_routeCache.find(subrouteId);
  if (it == m_routeCache.end())
    return;

  drape_ptr<SubrouteArrowsData> routeArrowsData = make_unique_dp<SubrouteArrowsData>();
  routeArrowsData->m_subrouteId = subrouteId;
  routeArrowsData->m_pivot = it->second.m_polyline.GetLimitRect().Center();
  routeArrowsData->m_recacheId = recacheId;
  RouteShape::CacheRouteArrows(textures, it->second.m_polyline, borders,
                               it->second.m_baseDepthIndex, *routeArrowsData.get());

  // Flush route arrows geometry.
  GLFunctions::glFlush();

  if (m_flushRouteArrowsFn != nullptr)
    m_flushRouteArrowsFn(std::move(routeArrowsData));
}
}  // namespace df
