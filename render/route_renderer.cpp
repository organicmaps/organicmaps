#include "render/route_renderer.hpp"
#include "render/route_shape.hpp"
#include "render/gpu_drawer.hpp"

#include "graphics/depth_constants.hpp"
#include "graphics/geometry_pipeline.hpp"
#include "graphics/resource.hpp"
#include "graphics/opengl/texture.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace rg
{

namespace
{

float const kHalfWidthInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  2.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 4.0f, 4.0f, 5.0f, 5.0f,
  //11   12    13    14    15    16    17    18    19     20
  6.0f, 6.0f, 7.0f, 7.0f, 7.0f, 7.0f, 8.0f, 10.0f, 24.0f, 36.0f
};

uint8_t const kAlphaValue[] =
{
  //1   2    3    4    5    6    7    8    9    10
  204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
  //11  12   13   14   15   16   17   18   19   20
  204, 204, 204, 204, 190, 180, 170, 160, 140, 120
};

int const kArrowAppearingZoomLevel = 14;

enum SegmentStatus
{
  OK = -1,
  NoSegment = -2
};

int const kInvalidGroup = -1;

// Checks for route segments for intersection with the distance [start; end].
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

// Finds the nearest appropriate route segment to the distance [start; end].
int FindNearestAvailableSegment(double start, double end, vector<RouteSegment> const & segments)
{
  double const kThreshold = 0.8;

  // check if distance intersects unavailable segment
  int index = CheckForIntersection(start, end, segments);
  if (index == SegmentStatus::OK)
    return SegmentStatus::OK;

  // find nearest available segment if necessary
  double const len = end - start;
  for (size_t i = index; i < segments.size(); i++)
  {
    double const factor = (segments[i].m_end - segments[i].m_start) / len;
    if (segments[i].m_isAvailable && factor > kThreshold)
      return static_cast<int>(i);
  }
  return SegmentStatus::NoSegment;
}

void ClipBorders(vector<ArrowBorders> & borders)
{
  auto invalidBorders = [](ArrowBorders const & borders)
  {
    return borders.m_groupIndex == kInvalidGroup;
  };
  borders.erase(remove_if(borders.begin(), borders.end(), invalidBorders), borders.end());
}

void MergeAndClipBorders(vector<ArrowBorders> & borders)
{
  // initial clipping
  ClipBorders(borders);

  if (borders.empty())
    return;

  // mark groups
  for (size_t i = 0; i + 1 < borders.size(); i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // merge groups
  int lastGroup = borders[0].m_groupIndex;
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
      borders[i].m_groupIndex = kInvalidGroup;
    }
  }
  borders[lastGroupIndex].m_endDistance = borders.back().m_endDistance;

  // clip groups
  ClipBorders(borders);
}

double CalculateLength(vector<m2::PointD> const & points)
{
  ASSERT_LESS(0, points.size(), ());
  double len = 0;
  for (size_t i = 0; i + 1 < points.size(); i++)
    len += (points[i + 1] - points[i]).Length();
  return len;
}

vector<m2::PointD> AddPoints(double offset, vector<m2::PointD> const & points, bool isTail)
{
  vector<m2::PointD> result;
  result.reserve(points.size() + 1);
  double len = 0;
  for (size_t i = 0; i + 1 < points.size(); i++)
  {
    double dist = (points[i + 1] - points[i]).Length();
    double l = len + dist;
    if (offset >= len && offset <= l)
    {
      double k = (offset - len) / dist;
      if (isTail)
      {
        result.push_back(points.front());
        result.push_back(points[i] + (points[i + 1] - points[i]) * k);
        result.insert(result.end(), points.begin() + i + 1, points.end());
      }
      else
      {
        result.insert(result.begin(), points.begin(), points.begin() + i + 1);
        result.push_back(points[i] + (points[i + 1] - points[i]) * k);
        result.push_back(points.back());
      }
      break;
    }
    len = l;
  }
  return result;
}

vector<m2::PointD> CalculatePoints(m2::PolylineD const & polyline, double start, double end,
                                   double headSize, double tailSize)
{
  vector<m2::PointD> result;
  result.reserve(polyline.GetSize() / 4);

  auto addIfNotExist = [&result](m2::PointD const & pnt)
  {
    if (result.empty() || result.back() != pnt)
      result.push_back(pnt);
  };

  vector<m2::PointD> const & path = polyline.GetPoints();
  double len = 0;
  bool started = false;
  for (size_t i = 0; i + 1 < path.size(); i++)
  {
    double dist = (path[i + 1] - path[i]).Length();
    if (fabs(dist) < 1e-5)
      continue;

    double l = len + dist;
    if (!started && start >= len && start <= l)
    {
      double k = (start - len) / dist;
      addIfNotExist(path[i] + (path[i + 1] - path[i]) * k);
      started = true;
    }
    if (!started)
    {
      len = l;
      continue;
    }

    if (end >= len && end <= l)
    {
      double k = (end - len) / dist;
      addIfNotExist(path[i] + (path[i + 1] - path[i]) * k);
      break;
    }
    else
    {
      addIfNotExist(path[i + 1]);
    }
    len = l;
  }
  if (result.empty())
    return result;

  result = AddPoints(tailSize, result, true /* isTail */);
  if (result.empty())
    return result;

  return AddPoints(CalculateLength(result) - headSize, result, false /* isTail */);
}

float NormColor(uint8_t value)
{
  return static_cast<float>(value) / numeric_limits<uint8_t>::max();
}

}

RouteRenderer::RouteRenderer()
  : m_arrowDisplayList(nullptr)
  , m_distanceFromBegin(0.0)
  , m_needClearGraphics(false)
  , m_needClearData(false)
  , m_waitForConstruction(false)
{
}

RouteRenderer::~RouteRenderer()
{
  ASSERT(m_routeGraphics.empty(), ());
  ASSERT(m_startRoutePoint.m_displayList == nullptr, ());
  ASSERT(m_finishRoutePoint.m_displayList == nullptr, ());
  ASSERT(m_arrowDisplayList == nullptr, ());
}

void RouteRenderer::SetRoutePoint(m2::PointD const & pt, bool start)
{
  RoutePoint & pnt = start ? m_startRoutePoint : m_finishRoutePoint;
  pnt.m_point = pt;
  pnt.m_needUpdate = true;
}

void RouteRenderer::Setup(m2::PolylineD const & routePolyline, vector<double> const & turns, graphics::Color const & color)
{
  if (!m_routeData.m_geometry.empty())
    m_needClearGraphics = true;

  RouteShape::PrepareGeometry(routePolyline, m_routeData);

  m_turns = turns;
  m_color = color;
  m_distanceFromBegin = 0.0;
  m_polyline = routePolyline;

  if (!m_startRoutePoint.IsVisible())
    SetRoutePoint(m_polyline.Front(), true /* start */);

  if (!m_finishRoutePoint.IsVisible())
    SetRoutePoint(m_polyline.Back(), false /* start */);

  m_waitForConstruction = true;
}

void RouteRenderer::ConstructRoute(graphics::Screen * dlScreen)
{
  ASSERT(m_routeGraphics.empty(), ());

  // texture
  uint32_t resID = dlScreen->findInfo(graphics::Icon::Info("route-arrow"));
  graphics::Resource const * res = dlScreen->fromID(resID);
  ASSERT(res != nullptr, ());
  shared_ptr<graphics::gl::BaseTexture> texture = dlScreen->pipeline(res->m_pipelineID).texture();
  float w = static_cast<float>(texture->width());
  float h = static_cast<float>(texture->height());
  m_arrowTextureRect = m2::RectF(res->m_texRect.minX() / w, res->m_texRect.minY() / h,
                                 res->m_texRect.maxX() / w, res->m_texRect.maxY() / h);

  // storages
  for (auto & geometry : m_routeData.m_geometry)
  {
    m_routeGraphics.emplace_back(RouteGraphics());
    RouteGraphics & graphics = m_routeGraphics.back();

    size_t vbSize = geometry.first.size() * sizeof(graphics::gl::RouteVertex);
    size_t ibSize = geometry.second.size() * sizeof(unsigned short);
    ASSERT_NOT_EQUAL(vbSize, 0, ());
    ASSERT_NOT_EQUAL(ibSize, 0, ());

    graphics.m_storage = graphics::gl::Storage(vbSize, ibSize);
    void * vbPtr = graphics.m_storage.m_vertices->lock();
    memcpy(vbPtr, geometry.first.data(), vbSize);
    graphics.m_storage.m_vertices->unlock();

    void * ibPtr = graphics.m_storage.m_indices->lock();
    memcpy(ibPtr, geometry.second.data(), ibSize);
    graphics.m_storage.m_indices->unlock();
  }

  size_t const arrowBufferSize = m_turns.size() * 500;
  m_arrowsStorage = graphics::gl::Storage(arrowBufferSize * sizeof(graphics::gl::RouteVertex),
                                          arrowBufferSize * sizeof(unsigned short));

  // display lists
  for (auto & graphics : m_routeGraphics)
  {
    graphics.m_displayList = dlScreen->createDisplayList();
    dlScreen->setDisplayList(graphics.m_displayList);
    dlScreen->drawRouteGeometry(texture, graphics.m_storage);
  }

  m_arrowDisplayList = dlScreen->createDisplayList();
  dlScreen->setDisplayList(m_arrowDisplayList);
  dlScreen->drawRouteGeometry(texture, m_arrowsStorage);

  dlScreen->setDisplayList(nullptr);

  m_waitForConstruction = false;
}

void RouteRenderer::ClearRouteGraphics(graphics::Screen * dlScreen)
{
  for (RouteGraphics & graphics : m_routeGraphics)
  {
    dlScreen->discardStorage(graphics.m_storage);
    graphics.m_storage = graphics::gl::Storage();
  }

  dlScreen->discardStorage(m_arrowsStorage);
  m_arrowsStorage = graphics::gl::Storage();

  dlScreen->clearRouteGeometry();

  DestroyDisplayLists();

  m_arrowBorders.clear();
  m_routeSegments.clear();
  m_arrowBuffer.Clear();

  m_needClearGraphics = false;
}

void RouteRenderer::ClearRouteData()
{
  m_routeData.Clear();
  m_needClearData = false;
}

void RouteRenderer::PrepareToShutdown()
{
  DestroyDisplayLists();

  m_arrowBorders.clear();
  m_routeSegments.clear();

  if (!m_routeData.m_geometry.empty())
    m_waitForConstruction = true;
}

void RouteRenderer::DestroyDisplayLists()
{
  m_routeGraphics.clear();

  DestroyRoutePointGraphics(true /* start */);
  DestroyRoutePointGraphics(false /* start */);

  if (m_arrowDisplayList != nullptr)
  {
    delete m_arrowDisplayList;
    m_arrowDisplayList = nullptr;
  }
}

void RouteRenderer::CreateRoutePointGraphics(graphics::Screen * dlScreen, bool start)
{
  RoutePoint & pnt = start ? m_startRoutePoint : m_finishRoutePoint;
  if (pnt.m_needUpdate)
  {
    DestroyRoutePointGraphics(start);

    pnt.m_displayList = dlScreen->createDisplayList();
    dlScreen->setDisplayList(pnt.m_displayList);
    dlScreen->drawSymbol(pnt.m_point, start ? "route_from" : "route_to", graphics::EPosCenter, graphics::routingFinishDepth);
    dlScreen->setDisplayList(nullptr);

    pnt.m_needUpdate = false;
  }
}

void RouteRenderer::DestroyRoutePointGraphics(bool start)
{
  if (start)
    m_startRoutePoint.Reset();
  else
    m_finishRoutePoint.Reset();
}

void RouteRenderer::InterpolateByZoom(ScreenBase const & screen, float & halfWidth, float & alpha, double & zoom) const
{
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / log(2.0)), 1.0, scales::UPPER_STYLE_SCALE + 1.0);
  zoom = trunc(zoomLevel);
  int const index = zoom - 1.0;
  float const lerpCoef = zoomLevel - zoom;

  if (index < scales::UPPER_STYLE_SCALE)
  {
    halfWidth = kHalfWidthInPixel[index] + lerpCoef * (kHalfWidthInPixel[index + 1] - kHalfWidthInPixel[index]);

    float const alpha1 = NormColor(kAlphaValue[index]);
    float const alpha2 = NormColor(kAlphaValue[index + 1]);
    alpha = alpha1 + lerpCoef * NormColor(alpha2 - alpha1);
  }
  else
  {
    halfWidth = kHalfWidthInPixel[scales::UPPER_STYLE_SCALE];
    alpha = NormColor(kAlphaValue[scales::UPPER_STYLE_SCALE]);
  }
}

void RouteRenderer::Render(graphics::Screen * dlScreen, ScreenBase const & screen)
{
  // clearing
  if (m_needClearData)
    ClearRouteData();

  if (m_needClearGraphics)
    ClearRouteGraphics(dlScreen);

  // construction
  if (m_waitForConstruction)
    ConstructRoute(dlScreen);

  // route points
  CreateRoutePointGraphics(dlScreen, true /* start */);
  CreateRoutePointGraphics(dlScreen, false /* start */);

  if (!m_routeGraphics.empty())
  {
    ASSERT_EQUAL(m_routeGraphics.size(), m_routeData.m_geometry.size(), ());

    // rendering
    dlScreen->clear(graphics::Color(), false, 1.0f, true);

    // interpolate values by zoom level
    double zoom = 0.0;
    float halfWidth = 0.0;
    float alpha = 0.0;
    InterpolateByZoom(screen, halfWidth, alpha, zoom);

    // set up uniforms
    graphics::UniformsHolder uniforms;
    uniforms.insertValue(graphics::ERouteColor, NormColor(m_color.r), NormColor(m_color.g), NormColor(m_color.b), alpha);
    uniforms.insertValue(graphics::ERouteHalfWidth, halfWidth, halfWidth * screen.GetScale());
    uniforms.insertValue(graphics::ERouteClipLength, m_distanceFromBegin);

    // render routes
    dlScreen->applyRouteStates();
    for (size_t i = 0; i < m_routeGraphics.size(); ++i)
    {
      RouteGraphics & graphics = m_routeGraphics[i];
      if (!screen.ClipRect().IsIntersect(m_routeData.m_boundingBoxes[i]))
        continue;

      size_t const indicesCount = graphics.m_storage.m_indices->size() / sizeof(unsigned short);
      dlScreen->drawDisplayList(graphics.m_displayList, screen.GtoPMatrix(), &uniforms, indicesCount);
    }

    // render arrows
    if (zoom >= kArrowAppearingZoomLevel)
      RenderArrow(dlScreen, halfWidth, screen);
  }

  dlScreen->applyStates();
  if (m_startRoutePoint.IsVisible())
    dlScreen->drawDisplayList(m_startRoutePoint.m_displayList, screen.GtoPMatrix());

  if (m_finishRoutePoint.IsVisible())
    dlScreen->drawDisplayList(m_finishRoutePoint.m_displayList, screen.GtoPMatrix());
}

void RouteRenderer::RenderArrow(graphics::Screen * dlScreen, float halfWidth, ScreenBase const & screen)
{
  double const arrowHalfWidth = halfWidth * arrowHeightFactor;
  double const glbArrowHalfWidth = arrowHalfWidth * screen.GetScale();
  double const arrowSize = 0.001;
  double const textureWidth = 2.0 * arrowHalfWidth * arrowAspect;

  // calculate arrows
  vector<ArrowBorders> arrowBorders;
  arrowBorders.reserve(m_arrowBorders.size());
  CalculateArrowBorders(screen.ClipRect(), arrowSize, screen.GetScale(),
                        textureWidth, glbArrowHalfWidth, arrowBorders);
  if (!arrowBorders.empty())
  {
    bool needRender = true;
    if (m_arrowBorders != arrowBorders)
    {
      m_arrowBorders.swap(arrowBorders);
      needRender = RecacheArrows();
    }

    if (needRender)
    {
      dlScreen->clear(graphics::Color(), false, 1.0f, true);
      dlScreen->applyRouteArrowStates();

      graphics::UniformsHolder uniforms;
      uniforms.insertValue(graphics::ERouteHalfWidth, arrowHalfWidth, glbArrowHalfWidth);
      uniforms.insertValue(graphics::ERouteTextureRect, m_arrowTextureRect.minX(), m_arrowTextureRect.minY(),
                           m_arrowTextureRect.maxX(), m_arrowTextureRect.maxY());

      dlScreen->drawDisplayList(m_arrowDisplayList, screen.GtoPMatrix(), &uniforms, m_arrowBuffer.m_indices.size());
    }
  }
}

bool RouteRenderer::RecacheArrows()
{
  m_arrowBuffer.Clear();
  for (size_t i = 0; i < m_arrowBorders.size(); i++)
  {
    RouteShape::PrepareArrowGeometry(m_arrowBorders[i].m_points,
                                     m_arrowBorders[i].m_startDistance, m_arrowBorders[i].m_endDistance,
                                     m_arrowBuffer);
  }

  size_t const vbSize = m_arrowBuffer.m_geometry.size() * sizeof(m_arrowBuffer.m_geometry[0]);
  size_t const ibSize = m_arrowBuffer.m_indices.size() * sizeof(m_arrowBuffer.m_indices[0]);
  if (vbSize == 0 && ibSize == 0)
    return false;

  if (m_arrowsStorage.m_vertices->size() < vbSize || m_arrowsStorage.m_indices->size() < ibSize)
  {
    LOG(LWARNING, ("Arrows VB/IB has insufficient size"));
    return false;
  }

  void * vbPtr = m_arrowsStorage.m_vertices->lock();
  memcpy(vbPtr, m_arrowBuffer.m_geometry.data(), vbSize);
  m_arrowsStorage.m_vertices->unlock();

  void * ibPtr = m_arrowsStorage.m_indices->lock();
  memcpy(ibPtr, m_arrowBuffer.m_indices.data(), ibSize);
  m_arrowsStorage.m_indices->unlock();

  return true;
}

void RouteRenderer::Clear()
{
  m_needClearGraphics = true;
  m_needClearData = true;
  m_distanceFromBegin = 0.0;
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength,
                                     vector<ArrowBorders> & arrowBorders)
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
  for (size_t i = 0; i < arrowBorders.size(); i++)
  {
    int headIndex = FindNearestAvailableSegment(arrowBorders[i].m_endDistance - glbHeadLength,
                                                arrowBorders[i].m_endDistance, m_routeSegments);
    if (headIndex != SegmentStatus::OK)
    {
      if (headIndex != SegmentStatus::NoSegment)
      {
        ASSERT_GREATER_OR_EQUAL(headIndex, 0, ());
        double const restDist = m_routeData.m_length - m_routeSegments[headIndex].m_start;
        if (restDist >= glbHeadLength)
          arrowBorders[i].m_endDistance = min(m_routeData.m_length, m_routeSegments[headIndex].m_start + glbHeadLength);
        else
          arrowBorders[i].m_groupIndex = kInvalidGroup;
      }
      else
      {
        arrowBorders[i].m_groupIndex = kInvalidGroup;
      }
      needMerge = true;
    }
  }

  // merge intersected borders
  if (needMerge)
    MergeAndClipBorders(arrowBorders);
}

void RouteRenderer::CalculateArrowBorders(m2::RectD const & clipRect, double arrowLength, double scale,
                                          double arrowTextureWidth, double joinsBoundsScalar,
                                          vector<ArrowBorders> & arrowBorders)
{
  if (m_turns.empty())
    return;

  double halfLen = 0.5 * arrowLength;
  double const glbTextureWidth = arrowTextureWidth * scale;
  double const glbTailLength = arrowTailSize * glbTextureWidth;
  double const glbHeadLength = arrowHeadSize * glbTextureWidth;

  double const halfTextureWidth = 0.5 * glbTextureWidth;
  if (halfLen < halfTextureWidth)
    halfLen = halfTextureWidth;

  // initial filling
  for (size_t i = 0; i < m_turns.size(); i++)
  {
    ArrowBorders borders;
    borders.m_groupIndex = static_cast<int>(i);
    borders.m_startDistance = max(0.0, m_turns[i] - halfLen * 0.8);
    borders.m_endDistance = min(m_routeData.m_length, m_turns[i] + halfLen * 1.2);
    borders.m_headSize = glbHeadLength;
    borders.m_tailSize = glbTailLength;

    if (borders.m_startDistance < m_distanceFromBegin)
      continue;

    arrowBorders.push_back(borders);
  }

  // merge intersected borders and clip them
  MergeAndClipBorders(arrowBorders);

  // apply joins bounds to prevent draw arrow's head on a join
  ApplyJoinsBounds(joinsBoundsScalar, glbHeadLength, arrowBorders);

  // check if arrow is outside clip rect
  for (size_t i = 0; i < arrowBorders.size(); i++)
  {
    arrowBorders[i].m_points = CalculatePoints(m_polyline,
                                               arrowBorders[i].m_startDistance,
                                               arrowBorders[i].m_endDistance,
                                               arrowBorders[i].m_headSize,
                                               arrowBorders[i].m_tailSize);
    bool outside = true;
    for (size_t j = 0; j < arrowBorders[i].m_points.size(); j++)
    {
      if (clipRect.IsPointInside(arrowBorders[i].m_points[j]))
      {
        outside = false;
        break;
      }
    }
    if (outside || arrowBorders[i].m_points.size() < 2)
    {
      arrowBorders[i].m_groupIndex = -1;
      continue;
    }
  }
  ClipBorders(arrowBorders);
}

} // namespace rg

