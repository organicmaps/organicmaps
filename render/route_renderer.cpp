#include "render/route_renderer.hpp"
#include "render/route_shape.hpp"
#include "render/gpu_drawer.hpp"

#include "graphics/geometry_pipeline.hpp"
#include "graphics/resource.hpp"
#include "graphics/opengl/texture.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace rg
{

namespace
{

float const halfWidthInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f,
  //11   12    13    14    15     16    17      18     19
  2.0f, 2.5f, 3.5f, 5.0f, 7.5f, 10.0f, 14.0f, 18.0f, 40.0f,
};

enum SegmentStatus
{
  OK = -1,
  NoSegment = -2
};

int const invalidGroup = -1;
int const arrowAppearingZoomLevel = 14;

int const arrowPartsCount = 3;
double const arrowHeightFactor = 96.0 / 36.0;
double const arrowAspect = 400.0 / 192.0;
double const arrowTailSize = 20.0 / 400.0;
double const arrowHeadSize = 124.0 / 400.0;

int CheckForIntersection(double start, double end, vector<RouteSegment> const & segments)
{
  for (size_t i = 0; i < segments.size(); i++)
  {
    if (segments[i].m_isAvailable)
      continue;

    if (start <= segments[i].m_end && end >= segments[i].m_start)
      return i;
  }
  return SegmentStatus::OK;
}

int FindNearestAvailableSegment(double start, double end, vector<RouteSegment> const & segments)
{
  double const threshold = 0.8;

  // check if distance intersects unavailable segment
  int index = CheckForIntersection(start, end, segments);
  if (index == SegmentStatus::OK)
    return SegmentStatus::OK;

  // find nearest available segment if necessary
  double const len = end - start;
  for (size_t i = index; i < segments.size(); i++)
  {
    double const factor = (segments[i].m_end - segments[i].m_start) / len;
    if (segments[i].m_isAvailable && factor > threshold)
      return static_cast<int>(i);
  }
  return SegmentStatus::NoSegment;
}

void MergeAndClipBorders(vector<ArrowBorders> & borders)
{
  auto invalidBorders = [](ArrowBorders const & borders)
  {
    return borders.m_groupIndex == invalidGroup;
  };

  // initial clipping
  borders.erase(remove_if(borders.begin(), borders.end(), invalidBorders), borders.end());

  if (borders.empty())
    return;

  // mark groups
  for (size_t i = 0; i < borders.size() - 1; i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // merge groups
  int lastGroup = 0;
  size_t lastGroupIndex = 0;
  for (size_t i = 1; i < borders.size(); i++)
  {
    if (borders[i].m_groupIndex != lastGroup)
    {
      borders[lastGroupIndex].m_endDistance = borders[i - 1].m_endDistance;
      lastGroupIndex = i;
      lastGroup = borders[i].m_groupIndex;
    }
    else
    {
      borders[i].m_groupIndex = invalidGroup;
    }
  }
  borders[lastGroupIndex].m_endDistance = borders.back().m_endDistance;

  // clip groups
  borders.erase(remove_if(borders.begin(), borders.end(), invalidBorders), borders.end());
}

float NormColor(uint8_t value)
{
  return static_cast<float>(value) / 255.0f;
}

}

RouteRenderer::RouteRenderer()
  : m_displayList(nullptr)
  , m_endOfRouteDisplayList(nullptr)
  , m_distanceFromBegin(0.0)
  , m_needClear(false)
  , m_waitForConstruction(false)
{}

void RouteRenderer::Setup(m2::PolylineD const & routePolyline, vector<double> const & turns, graphics::Color const & color)
{
  RouteShape::PrepareGeometry(routePolyline, m_routeData);
  m_turns = turns;
  m_color = color;
  m_endOfRoutePoint = routePolyline.GetPoints().back();
  m_waitForConstruction = true;
}

void RouteRenderer::ConstructRoute(graphics::Screen * dlScreen)
{
  // texture
  uint32_t resID = dlScreen->findInfo(graphics::Icon::Info("route-arrow"));
  graphics::Resource const * res = dlScreen->fromID(resID);
  ASSERT(res != nullptr, ());
  shared_ptr<graphics::gl::BaseTexture> texture = dlScreen->pipeline(res->m_pipelineID).texture();
  float w = static_cast<float>(texture->width());
  float h = static_cast<float>(texture->height());
  m_arrowTextureRect = m2::RectF(res->m_texRect.minX() / w, res->m_texRect.minY() / h,
                                 res->m_texRect.maxX() / w, res->m_texRect.maxY() / h);

  // storage
  size_t vbSize = m_routeData.m_geometry.size() * sizeof(graphics::gl::RouteVertex);
  size_t ibSize = m_routeData.m_indices.size() * sizeof(unsigned short);

  m_storage = graphics::gl::Storage(vbSize, ibSize);
  void * vbPtr = m_storage.m_vertices->lock();
  memcpy(vbPtr, m_routeData.m_geometry.data(), vbSize);
  m_routeData.m_geometry.clear();

  m_storage.m_vertices->unlock();
  void * ibPtr = m_storage.m_indices->lock();
  memcpy(ibPtr, m_routeData.m_indices.data(), ibSize);
  m_storage.m_indices->unlock();
  m_routeData.m_indices.clear();

  // display lists
  dlScreen->beginFrame();

  m_displayList = dlScreen->createDisplayList();
  dlScreen->setDisplayList(m_displayList);
  dlScreen->drawRouteGeometry(texture, m_storage);

  m_endOfRouteDisplayList = dlScreen->createDisplayList();
  dlScreen->setDisplayList(m_endOfRouteDisplayList);
  dlScreen->drawSymbol(m_endOfRoutePoint, "route_to", graphics::EPosCenter, 0);

  dlScreen->setDisplayList(nullptr);
  dlScreen->endFrame();
}

void RouteRenderer::ClearRoute(graphics::Screen * dlScreen)
{
  dlScreen->discardStorage(m_storage);
  dlScreen->clearRouteGeometry();
  m_storage = graphics::gl::Storage();

  if (m_displayList != nullptr)
  {
    delete m_displayList;
    m_displayList = nullptr;
  }

  if (m_endOfRouteDisplayList != nullptr)
  {
    delete m_endOfRouteDisplayList;
    m_endOfRouteDisplayList = nullptr;
  }

  m_arrowBorders.clear();
  m_routeSegments.clear();
}

float RouteRenderer::CalculateRouteHalfWidth(ScreenBase const & screen, double & zoom) const
{
  float halfWidth = 0.0;
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / log(2.0)), 1.0, scales::UPPER_STYLE_SCALE);
  zoom = trunc(zoomLevel);
  int const index = zoom - 1.0;
  float const lerpCoef = zoomLevel - zoom;

  if (index < scales::UPPER_STYLE_SCALE - 1)
    halfWidth = halfWidthInPixel[index] + lerpCoef * (halfWidthInPixel[index + 1] - halfWidthInPixel[index]);
  else
    halfWidth = halfWidthInPixel[index];

  return halfWidth;
}

void RouteRenderer::Render(shared_ptr<PaintEvent> const & e, ScreenBase const & screen)
{
  graphics::Screen * dlScreen = GPUDrawer::GetScreen(e->drawer());

  // clearing
  if (m_needClear)
  {
    ClearRoute(dlScreen);
    m_needClear = false;
  }

  // construction
  if (m_waitForConstruction)
  {
    ConstructRoute(dlScreen);
    m_waitForConstruction = false;
  }

  if (m_displayList == nullptr)
    return;

  // rendering
  dlScreen->clear(graphics::Color(), false, 1.0f, true);

  double zoom = 0.0;
  float const halfWidth = CalculateRouteHalfWidth(screen, zoom);

  // set up uniforms
  graphics::UniformsHolder uniforms;
  uniforms.insertValue(graphics::ERouteColor, NormColor(m_color.r), NormColor(m_color.g),
                       NormColor(m_color.b), NormColor(m_color.a));
  uniforms.insertValue(graphics::ERouteHalfWidth, halfWidth, halfWidth * screen.GetScale());
  uniforms.insertValue(graphics::ERouteClipLength, m_distanceFromBegin);

  // render routes
  dlScreen->applyRouteStates();
  dlScreen->drawDisplayList(m_displayList, screen.GtoPMatrix(), &uniforms);

  // render arrows
  if (zoom >= arrowAppearingZoomLevel)
    RenderArrow(dlScreen, halfWidth, screen);

  dlScreen->applyStates();
  dlScreen->drawDisplayList(m_endOfRouteDisplayList, screen.GtoPMatrix());
}

void RouteRenderer::RenderArrow(graphics::Screen * dlScreen, float halfWidth, ScreenBase const & screen)
{
  double const arrowHalfWidth = halfWidth * arrowHeightFactor;
  double const glbArrowHalfWidth = arrowHalfWidth * screen.GetScale();
  double const arrowSize = 0.001;
  double const textureWidth = 2.0 * arrowHalfWidth * arrowAspect;

  // calculate arrows
  m_arrowBorders.clear();
  CalculateArrowBorders(arrowSize, screen.GetScale(), textureWidth, glbArrowHalfWidth);

  // split arrow's data by 16-elements buckets
  array<float, 16> borders;
  borders.fill(0.0f);
  size_t const elementsCount = borders.size();

  if (!m_arrowBorders.empty())
    dlScreen->applyRouteArrowStates();

  size_t index = 0;
  for (size_t i = 0; i < m_arrowBorders.size(); i++)
  {
    borders[index++] = m_arrowBorders[i].m_startDistance;
    borders[index++] = m_arrowBorders[i].m_startTexCoord;
    borders[index++] = m_arrowBorders[i].m_endDistance;
    borders[index++] = m_arrowBorders[i].m_endTexCoord;

    // render arrow's parts
    if (index == elementsCount || i == m_arrowBorders.size() - 1)
    {
      index = 0;

      graphics::UniformsHolder uniforms;
      uniforms.insertValue(graphics::ERouteHalfWidth, arrowHalfWidth, glbArrowHalfWidth);
      uniforms.insertValue(graphics::ERouteTextureRect, m_arrowTextureRect.minX(), m_arrowTextureRect.minY(),
                           m_arrowTextureRect.maxX(), m_arrowTextureRect.maxY());
      uniforms.insertValue(graphics::ERouteArrowBorders, math::Matrix<float, 4, 4>(borders.data()));
      borders.fill(0.0f);

      dlScreen->drawDisplayList(m_displayList, screen.GtoPMatrix(), &uniforms);
    }
  }
}

void RouteRenderer::Clear()
{
  m_needClear = true;
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength)
{
  m_routeSegments.clear();
  m_routeSegments.reserve(2 * m_routeData.m_joinsBounds.size() + 1);

  // construct route's segments
  m_routeSegments.emplace_back(0.0, 0.0, true /* m_isAvailable */);
  for (size_t i = 0; i < m_routeData.m_joinsBounds.size(); i++)
  {
    double const start = m_routeData.m_joinsBounds[i].m_offset +
                         m_routeData.m_joinsBounds[i].m_start * joinsBoundsScalar;
    double const end = m_routeData.m_joinsBounds[i].m_offset +
                       m_routeData.m_joinsBounds[i].m_end * joinsBoundsScalar;

    m_routeSegments.back().m_end = start;
    m_routeSegments.emplace_back(start, end, false /* m_isAvailable */);

    m_routeSegments.emplace_back(end, 0.0, true /* m_isAvailable */);
  }
  m_routeSegments.back().m_end = m_routeData.m_length;

  // shift head of arrow if necessary
  bool needMerge = false;
  for (size_t i = 0; i < m_arrowBorders.size(); i++)
  {
    int headIndex = FindNearestAvailableSegment(m_arrowBorders[i].m_endDistance - glbHeadLength,
                                                m_arrowBorders[i].m_endDistance, m_routeSegments);
    if (headIndex != SegmentStatus::OK)
    {
      double const restDist = m_routeData.m_length - m_routeSegments[headIndex].m_start;
      if (headIndex == SegmentStatus::NoSegment || restDist < glbHeadLength)
        m_arrowBorders[i].m_groupIndex = invalidGroup;
      else
        m_arrowBorders[i].m_endDistance = min(m_routeData.m_length, m_routeSegments[headIndex].m_start + glbHeadLength);
      needMerge = true;
    }
  }

  // merge intersected borders
  if (needMerge)
    MergeAndClipBorders(m_arrowBorders);
}

void RouteRenderer::CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth, double joinsBoundsScalar)
{
  if (m_turns.empty())
    return;

  double halfLen = 0.5 * arrowLength;
  double const glbTextureWidth = arrowTextureWidth * scale;
  double const glbTailLength = arrowTailSize * glbTextureWidth;
  double const glbHeadLength = arrowHeadSize * glbTextureWidth;

  m_arrowBorders.reserve(m_turns.size() * arrowPartsCount);

  double const halfTextureWidth = 0.5 * glbTextureWidth;
  if (halfLen < halfTextureWidth)
    halfLen = halfTextureWidth;

  // initial filling
  for (size_t i = 0; i < m_turns.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = (int)i;
    arrowBorders.m_startDistance = max(0.0, m_turns[i] - halfLen * 0.8);
    arrowBorders.m_endDistance = min(m_routeData.m_length, m_turns[i] + halfLen * 1.2);

    if (arrowBorders.m_startDistance < m_distanceFromBegin)
      continue;

    m_arrowBorders.push_back(arrowBorders);
  }

  // merge intersected borders and clip them
  MergeAndClipBorders(m_arrowBorders);

  // apply joins bounds to prevent draw arrow's head on a join
  ApplyJoinsBounds(joinsBoundsScalar, glbHeadLength);

  // divide to parts of arrow
  size_t const bordersSize = m_arrowBorders.size();
  for (size_t i = 0; i < bordersSize; i++)
  {
    float const startDistance = m_arrowBorders[i].m_startDistance;
    float const endDistance = m_arrowBorders[i].m_endDistance;

    m_arrowBorders[i].m_endDistance = startDistance + glbTailLength;
    m_arrowBorders[i].m_startTexCoord = 0.0;
    m_arrowBorders[i].m_endTexCoord = arrowTailSize;

    ArrowBorders arrowHead;
    arrowHead.m_startDistance = endDistance - glbHeadLength;
    arrowHead.m_endDistance = endDistance;
    arrowHead.m_startTexCoord = 1.0 - arrowHeadSize;
    arrowHead.m_endTexCoord = 1.0;
    m_arrowBorders.push_back(arrowHead);

    ArrowBorders arrowBody;
    arrowBody.m_startDistance = m_arrowBorders[i].m_endDistance;
    arrowBody.m_endDistance = arrowHead.m_startDistance;
    arrowBody.m_startTexCoord = m_arrowBorders[i].m_endTexCoord;
    arrowBody.m_endTexCoord = arrowHead.m_startTexCoord;
    m_arrowBorders.push_back(arrowBody);
  }
}

} // namespace rg

