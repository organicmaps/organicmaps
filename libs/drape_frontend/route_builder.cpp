#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

#include "drape/graphics_context.hpp"
#include "drape/texture_manager.hpp"

namespace df
{
RouteBuilder::RouteBuilder(FlushFn && flushFn, FlushArrowsFn && flushArrowsFn, FlushMarkersFn && flushMarkersFn)
  : m_flushFn(std::move(flushFn))
  , m_flushArrowsFn(std::move(flushArrowsFn))
  , m_flushMarkersFn(std::move(flushMarkersFn))
{}

void RouteBuilder::Build(ref_ptr<dp::GraphicsContext> context, dp::DrapeID subrouteId, SubrouteConstPtr subroute,
                         ref_ptr<dp::TextureManager> textures, int recacheId)
{
  RouteCacheData cacheData;
  cacheData.m_polyline = subroute->m_polyline;
  cacheData.m_baseDepthIndex = subroute->m_baseDepthIndex;
  m_routeCache[subrouteId] = std::move(cacheData);

  std::vector<drape_ptr<df::SubrouteData>> subrouteData;
  subrouteData.reserve(subroute->m_style.size());

  ASSERT(!subroute->m_style.empty(), ());
  for (size_t styleIndex = 0; styleIndex < subroute->m_style.size(); ++styleIndex)
    subrouteData.push_back(RouteShape::CacheRoute(context, subrouteId, subroute, styleIndex, recacheId));

  auto markersData = RouteShape::CacheMarkers(context, subrouteId, subroute, recacheId, textures);

  // Flush route geometry.
  context->Flush();

  if (m_flushFn != nullptr)
  {
    for (auto & data : subrouteData)
      m_flushFn(std::move(data));
    subrouteData.clear();
  }

  if (m_flushMarkersFn != nullptr && markersData != nullptr)
    m_flushMarkersFn(std::move(markersData));
}

void RouteBuilder::ClearRouteCache()
{
  m_routeCache.clear();
}

void RouteBuilder::BuildArrows(ref_ptr<dp::GraphicsContext> context, dp::DrapeID subrouteId,
                               std::vector<ArrowBorders> const & borders, ref_ptr<dp::TextureManager> textures,
                               int recacheId)
{
  auto it = m_routeCache.find(subrouteId);
  if (it == m_routeCache.end())
    return;

  drape_ptr<SubrouteArrowsData> routeArrowsData = make_unique_dp<SubrouteArrowsData>();
  routeArrowsData->m_subrouteId = subrouteId;
  routeArrowsData->m_pivot = it->second.m_polyline.GetLimitRect().Center();
  routeArrowsData->m_recacheId = recacheId;
  RouteShape::CacheRouteArrows(context, textures, it->second.m_polyline, borders, it->second.m_baseDepthIndex,
                               *routeArrowsData.get());

  // Flush route arrows geometry.
  context->Flush();

  if (m_flushArrowsFn != nullptr)
    m_flushArrowsFn(std::move(routeArrowsData));
}
}  // namespace df
