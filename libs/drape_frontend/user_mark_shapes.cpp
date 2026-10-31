#include "drape_frontend/user_mark_shapes.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/font_constants.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/scales.hpp"

#include "kml/area_data.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/logging.hpp"

#include "3party/earcut/earcut.hpp"
#include "3party/libtess2/Include/tesselator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include <boost/geometry/algorithms/correct.hpp>
#include <boost/geometry/algorithms/is_valid.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace mapbox::util
{
template <>
struct nth<0, m2::PointD>
{
  static double get(m2::PointD const & p) { return p.x; }
};

template <>
struct nth<1, m2::PointD>
{
  static double get(m2::PointD const & p) { return p.y; }
};
}  // namespace mapbox::util

namespace df
{
namespace
{
std::array<double, 20> constexpr kLineWidthZoomFactor = {
    // 1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19   20
    0.3, 0.3, 0.3, 0.4, 0.5, 0.6, 0.7, 0.7, 0.7, 0.7, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

int constexpr kTrackDirectionMinZoom = 11;
int constexpr kTrackEndpointMinZoom = 8;
float constexpr kTrackArrowOffset = 48.0f;
float constexpr kTrackArrowStep = 96.0f;
// Chevron size in dp, as in track-direction-arrow-*.svg; atlas regions add 2 px of padding per side.
float constexpr kTrackArrowLength = 7.0f;
float constexpr kTrackArrowHeight = 4.0f;
float constexpr kTrackArrowMaxHeight = 8.0f;
float constexpr kTrackArrowInset = 0.25f;
float constexpr kTrackArrowMinScale = 0.75f;
float constexpr kTrackArrowSearchRadius = 24.0f;
float constexpr kTrackEndpointRadius = 5.0f;
float constexpr kTrackEndpointOutlineWidth = 1.5f;
float constexpr kTrackCombinedEndpointDistance = 12.0f;
float constexpr kTrackShapeDepthOffset = 1.0f;
char const kTrackDirectionBlackSymbol[] = "track-direction-arrow-black";
char const kTrackDirectionWhiteSymbol[] = "track-direction-arrow-white";

enum class TrackEndpoint
{
  Start,
  Finish,
  StartAndFinish
};

float LinearizeColorComponent(float component)
{
  return component <= 0.04045f ? component / 12.92f : std::pow((component + 0.055f) / 1.055f, 2.4f);
}

dp::Color GetContrastingColor(dp::Color const & color)
{
  float const luminance = 0.2126f * LinearizeColorComponent(color.GetRedF()) +
                          0.7152f * LinearizeColorComponent(color.GetGreenF()) +
                          0.0722f * LinearizeColorComponent(color.GetBlueF());
  // Thin frames visually sink into saturated mid-dark colors before the equal-contrast threshold is reached.
  // Prefer white up to this point; it still has at least 3.5:1 contrast, while looking clearer on pure red.
  return luminance > 0.25f ? dp::Color::Black() : dp::Color::White();
}

dp::Color MakeOpaque(dp::Color const & color)
{
  return {color.GetRed(), color.GetGreen(), color.GetBlue(), 255};
}

float GetNextArrowOffset(double sourceOffset, double pixelToGlobalScale, float firstOffset, float step)
{
  ASSERT_GREATER(pixelToGlobalScale, 0.0, ());
  ASSERT_GREATER(step, 0.0f, ());

  double const sourceOffsetInPixels = sourceOffset / pixelToGlobalScale;
  if (sourceOffsetInPixels <= firstOffset)
    return static_cast<float>(firstOffset - sourceOffsetInPixels);

  double const phase = std::fmod(sourceOffsetInPixels - firstOffset, step);
  double constexpr kEps = 1e-6;
  return phase < kEps || step - phase < kEps ? 0.0f : static_cast<float>(step - phase);
}

void DrawTrackEndpoint(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                       ref_ptr<dp::TextureManager> textures, m2::PointD const & point, TrackEndpoint endpoint,
                       dp::Color const & trackColor, float depth, DepthLayer depthLayer, float visualScale,
                       dp::Batcher & batcher)
{
  ColoredSymbolViewParams params;
  params.m_tileCenter = tileKey.GetWrappedDataRect().Center();
  params.m_depth = depth;
  params.m_depthLayer = depthLayer;
  params.m_color = MakeOpaque(trackColor);
  params.m_outlineColor = GetContrastingColor(trackColor);
  params.m_outlineWidth = kTrackEndpointOutlineWidth * visualScale;

  if (endpoint == TrackEndpoint::Start)
  {
    params.m_shape = ColoredSymbolViewParams::Shape::Circle;
    params.m_radiusInPixels = kTrackEndpointRadius * visualScale;
  }
  else
  {
    params.m_shape = ColoredSymbolViewParams::Shape::Rectangle;
    float const size = 2.0f * kTrackEndpointRadius * visualScale;
    params.m_sizeInPixels = {size, size};
  }

  ColoredSymbolShape(point, params, tileKey, 0 /* textIndex */, false /* needOverlay */)
      .Draw(context, make_ref(&batcher), textures);

  if (endpoint != TrackEndpoint::StartAndFinish)
    return;

  // A dot inside the finish square keeps co-located loop endpoints distinguishable without stacking two full marks.
  params.m_shape = ColoredSymbolViewParams::Shape::Circle;
  params.m_radiusInPixels = 1.75f * visualScale;
  params.m_color = params.m_outlineColor;
  params.m_outlineWidth = 0.0f;
  params.m_depth += kTrackShapeDepthOffset;
  ColoredSymbolShape(point, params, tileKey, 1 /* textIndex */, false /* needOverlay */)
      .Draw(context, make_ref(&batcher), textures);
}

template <typename TCreateVector>
void AlignFormingNormals(TCreateVector const & fn, dp::Anchor anchor, dp::Anchor first, dp::Anchor second,
                         glsl::vec2 & firstNormal, glsl::vec2 & secondNormal)
{
  firstNormal = fn();
  secondNormal = -firstNormal;
  if ((anchor & second) != 0)
  {
    firstNormal *= 2;
    secondNormal = glsl::vec2(0.0, 0.0);
  }
  else if ((anchor & first) != 0)
  {
    firstNormal = glsl::vec2(0.0, 0.0);
    secondNormal *= 2;
  }
}

void AlignHorizontal(float halfWidth, dp::Anchor anchor, glsl::vec2 & left, glsl::vec2 & right)
{
  AlignFormingNormals([&halfWidth] { return glsl::vec2(-halfWidth, 0.0f); }, anchor, dp::Left, dp::Right, left, right);
}

void AlignVertical(float halfHeight, dp::Anchor anchor, glsl::vec2 & up, glsl::vec2 & down)
{
  AlignFormingNormals([&halfHeight] { return glsl::vec2(0.0f, -halfHeight); }, anchor, dp::Top, dp::Bottom, up, down);
}

struct UserPointVertex : public gpu::BaseVertex
{
  using TNormalAndAnimateOrZ = glsl::vec3;
  using TTexCoord = glsl::vec4;
  using TColor = glsl::vec4;
  using TAnimateOrZ = float;

  UserPointVertex() = default;
  UserPointVertex(TPosition const & pos, TNormalAndAnimateOrZ const & normalAndAnimateOrZ, TTexCoord const & texCoord,
                  TColor const & color)
    : m_position(pos)
    , m_normalAndAnimateOrZ(normalAndAnimateOrZ)
    , m_texCoord(texCoord)
    , m_color(color)
  {}

  static dp::BindingInfo GetBinding()
  {
    dp::BindingInfo info(4);
    uint8_t offset = 0;
    offset += dp::FillDecl<TPosition, UserPointVertex>(0, "a_position", info, offset);
    offset += dp::FillDecl<TNormalAndAnimateOrZ, UserPointVertex>(1, "a_normalAndAnimateOrZ", info, offset);
    offset += dp::FillDecl<TTexCoord, UserPointVertex>(2, "a_texCoords", info, offset);
    /*offset += */ dp::FillDecl<TColor, UserPointVertex>(3, "a_color", info, offset);

    return info;
  }

  TPosition m_position;
  TNormalAndAnimateOrZ m_normalAndAnimateOrZ;
  TTexCoord m_texCoord;
  TColor m_color;
};

std::string GetSymbolNameForZoomLevel(ref_ptr<UserPointMark::SymbolNameZoomInfo> symbolNames, TileKey const & tileKey)
{
  if (!symbolNames)
    return {};

  for (auto itName = symbolNames->crbegin(); itName != symbolNames->crend(); ++itName)
    if (itName->first <= tileKey.m_zoomLevel)
      return itName->second;
  return {};
}

m2::PointF GetSymbolOffsetForZoomLevel(ref_ptr<UserPointMark::SymbolOffsets> symbolOffsets, TileKey const & tileKey)
{
  if (!symbolOffsets)
    return m2::PointF::Zero();

  CHECK_GREATER(tileKey.m_zoomLevel, 0, ());
  CHECK_LESS_OR_EQUAL(tileKey.m_zoomLevel, scales::UPPER_STYLE_SCALE, ());

  auto const offsetIndex = static_cast<size_t>(tileKey.m_zoomLevel - 1);
  return symbolOffsets->operator[](offsetIndex);
}

void GenerateColoredSymbolShapes(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                                 UserMarkRenderParams const & renderInfo, TileKey const & tileKey,
                                 m2::PointD const & tileCenter, m2::PointF const & symbolOffset,
                                 m2::PointF & symbolSize, dp::Batcher & batcher)
{
  m2::PointF sizeInc(0.0f, 0.0f);
  m2::PointF offset(0.0f, 0.0f);
  UserPointMark::SymbolSizes symbolSizesInc;

  auto const isTextBg = renderInfo.m_coloredSymbols->m_addTextSize;

  if (isTextBg)
  {
    CHECK(renderInfo.m_titleDecl, ());
    auto const & titleDecl = renderInfo.m_titleDecl->operator[](0);
    auto const textMetrics = textures->ShapeSingleTextLine(titleDecl.m_primaryText, titleDecl.m_primaryLang, nullptr);
    auto const & vparams = VisualParams::Instance();
    auto const fontScale = static_cast<float>(vparams.GetFontScale() * vparams.GetVisualScale());
    float const textRatio = titleDecl.m_primaryTextFont.m_size * fontScale / dp::kBaseFontSizePixels;

    sizeInc.x = textMetrics.m_lineWidthInPixels * textRatio;
    sizeInc.y = textMetrics.m_maxLineHeightInPixels * textRatio;

    if (renderInfo.m_symbolSizes != nullptr)
    {
      symbolSizesInc.reserve(renderInfo.m_symbolSizes->size());
      for (auto const & sz : *renderInfo.m_symbolSizes)
        symbolSizesInc.push_back(sz + sizeInc);
    }

    offset = StraightTextLayout::GetSymbolBasedTextOffset(symbolSize, titleDecl.m_anchor, renderInfo.m_anchor);
  }

  ColoredSymbolViewParams params;
  if (renderInfo.m_coloredSymbols->m_isSymbolStub)
  {
    params.m_anchor = renderInfo.m_anchor;
    params.m_color = dp::Color::Transparent();
    params.m_shape = ColoredSymbolViewParams::Shape::Rectangle;
    params.m_sizeInPixels = symbolSize;
    params.m_offset = symbolOffset;
  }
  else
  {
    for (auto const & e : renderInfo.m_coloredSymbols->m_zoomInfo)
    {
      if (e.first <= tileKey.m_zoomLevel)
      {
        params = e.second;
        break;
      }
    }
  }

  // Assign ids after fetching params from map above.
  params.m_featureId = renderInfo.m_featureId;
  params.m_markId = renderInfo.m_markId;

  m2::PointF coloredSize(0.0f, 0.0f);
  if (params.m_shape == ColoredSymbolViewParams::Shape::Circle)
  {
    params.m_radiusInPixels = params.m_radiusInPixels + std::max(sizeInc.x, sizeInc.y) / 2.0f;
    coloredSize = m2::PointF(params.m_radiusInPixels * 2.0f, params.m_radiusInPixels * 2.0f);
  }
  else
  {
    params.m_sizeInPixels = params.m_sizeInPixels + sizeInc;
    coloredSize = params.m_sizeInPixels;
  }
  if (!isTextBg)
    symbolSize = m2::PointF(std::max(coloredSize.x, symbolSize.x), std::max(coloredSize.y, symbolSize.y));

  params.m_tileCenter = tileCenter;
  params.m_depthTestEnabled = renderInfo.m_depthTestEnabled;
  params.m_depth = renderInfo.m_depth;
  params.m_depthLayer = renderInfo.m_depthLayer;
  params.m_minVisibleScale = renderInfo.m_minZoom;
  params.m_specialDisplacement = renderInfo.m_displacement;
  params.m_specialPriority = renderInfo.m_priority;
  params.m_offset += offset;
  if (renderInfo.m_symbolSizes != nullptr)
  {
    ColoredSymbolShape(renderInfo.m_pivot, params, tileKey, kStartUserMarkOverlayIndex + renderInfo.m_index,
                       isTextBg ? symbolSizesInc : *renderInfo.m_symbolSizes.get())
        .Draw(context, &batcher, textures);
  }
  else
  {
    ColoredSymbolShape(renderInfo.m_pivot, params, tileKey, kStartUserMarkOverlayIndex + renderInfo.m_index,
                       renderInfo.m_coloredSymbols->m_needOverlay)
        .Draw(context, &batcher, textures);
  }
}

void GeneratePoiSymbolShape(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                            UserMarkRenderParams const & renderInfo, TileKey const & tileKey,
                            m2::PointD const & tileCenter, std::string const & symbolName,
                            m2::PointF const & symbolOffset, dp::Batcher & batcher)
{
  PoiSymbolViewParams params;
  params.m_featureId = renderInfo.m_featureId;
  params.m_markId = renderInfo.m_markId;
  params.m_tileCenter = tileCenter;
  params.m_depthTestEnabled = renderInfo.m_depthTestEnabled;
  params.m_depth = renderInfo.m_depth;
  params.m_depthLayer = renderInfo.m_depthLayer;
  params.m_minVisibleScale = renderInfo.m_minZoom;
  params.m_specialDisplacement = renderInfo.m_displacement;
  params.m_specialPriority = renderInfo.m_priority;
  params.m_symbolName = symbolName;
  params.m_anchor = renderInfo.m_anchor;
  params.m_offset = symbolOffset;

  bool const hasColoredOverlay = renderInfo.m_coloredSymbols != nullptr && renderInfo.m_coloredSymbols->m_needOverlay;
  params.m_startOverlayRank = hasColoredOverlay ? dp::OverlayRank1 : dp::OverlayRank0;

  PoiSymbolShape(renderInfo.m_pivot, params, tileKey, kStartUserMarkOverlayIndex + renderInfo.m_index)
      .Draw(context, &batcher, textures);
}

void GenerateTextShapes(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                        UserMarkRenderParams const & renderInfo, TileKey const & tileKey, m2::PointD const & tileCenter,
                        m2::PointF const & symbolOffset, m2::PointF const & symbolSize, dp::Batcher & batcher)
{
  if (renderInfo.m_minTitleZoom > tileKey.m_zoomLevel)
    return;

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  for (auto const & titleDecl : *renderInfo.m_titleDecl)
  {
    if (titleDecl.m_primaryText.empty())
      continue;

    TextViewParams params;
    params.m_featureId = renderInfo.m_featureId;
    params.m_markId = renderInfo.m_markId;
    params.m_tileCenter = tileCenter;
    params.m_titleDecl = titleDecl;

    // Here we use visual scale to adapt texts sizes and offsets
    // to different screen resolutions and DPI.
    params.m_titleDecl.m_primaryTextFont.m_size *= vs;
    params.m_titleDecl.m_secondaryTextFont.m_size *= vs;
    params.m_titleDecl.m_primaryOffset *= vs;
    params.m_titleDecl.m_secondaryOffset *= vs;

    params.m_depthTestEnabled = renderInfo.m_depthTestEnabled;
    params.m_depth = renderInfo.m_depth;
    params.m_depthLayer = renderInfo.m_titleDepthLayer;
    params.m_minVisibleScale = renderInfo.m_minZoom;
    params.m_startOverlayRank = dp::OverlayRank0;

    uint32_t const overlayIndex = kStartUserMarkOverlayIndex + renderInfo.m_index;
    if (renderInfo.m_hasTitlePriority)
    {
      params.m_specialDisplacement = renderInfo.m_displacement;
      params.m_specialPriority = renderInfo.m_priority;

      if (renderInfo.m_symbolNames != nullptr && renderInfo.m_symbolIsPOI)
        params.m_startOverlayRank++;

      if (renderInfo.m_coloredSymbols != nullptr && renderInfo.m_coloredSymbols->m_needOverlay)
        params.m_startOverlayRank++;

      ASSERT_LESS(params.m_startOverlayRank, dp::OverlayRanksCount, ());
    }

    if (renderInfo.m_symbolSizes != nullptr)
    {
      TextShape(renderInfo.m_pivot, params, tileKey, *renderInfo.m_symbolSizes,
                m2::PointF(0.0f, 0.0f) /* symbolOffset */, renderInfo.m_anchor, overlayIndex)
          .Draw(context, &batcher, textures);
    }
    else
    {
      TextShape(renderInfo.m_pivot, params, tileKey, symbolSize, symbolOffset, renderInfo.m_anchor, overlayIndex)
          .Draw(context, &batcher, textures);
    }
  }
}

m2::SharedSpline SimplifySpline(m2::SharedSpline const & in, double minSqrLength)
{
  auto spline = std::make_unique<m2::Spline>(in->GetSize());

  m2::PointD lastAddedPoint;
  for (auto const & point : in->GetPath())
  {
    if (spline->GetSize() > 1 && point.SquaredLength(lastAddedPoint) < minSqrLength)
    {
      spline->ReplacePoint(point);
    }
    else
    {
      spline->AddPoint(point);
      lastAddedPoint = point;
    }
  }
  return spline;
}

// SimplifySpline collapses a closed ring smaller than its step into a point, while the ring's stroke is visible.
m2::SharedSpline SimplifyRing(m2::SharedSpline const & ring, double minSqrLength)
{
  if (auto simplified = SimplifySpline(ring, minSqrLength); simplified->GetSize() > 2)
    return simplified;

  // A few vertices keep the ring drawable.
  size_t constexpr kMaxPoints = 8;
  auto const & path = ring->GetPath();
  size_t const step = std::max<size_t>(1, path.size() / kMaxPoints);
  auto spline = std::make_unique<m2::Spline>(path.size() / step + 2);
  for (size_t i = 0; i < path.size(); i += step)
    spline->AddPoint(path[i]);
  spline->AddPoint(path.back());
  return m2::SharedSpline(std::move(spline));
}

LineLayer const & GetTopLayer(UserLineRenderParams const & renderInfo)
{
  ASSERT(!renderInfo.m_layers.empty(), ());
  return *std::ranges::max_element(renderInfo.m_layers, {}, &LineLayer::m_depth);
}

std::string GetBackgroundSymbolName(std::string const & symbolName)
{
  char const * kDelimiter = "-";
  auto const tokens = strings::Tokenize(symbolName, kDelimiter);
  if (tokens.size() < 2 || tokens.size() > 3)
    return {};

  std::string res;
  res.append(tokens[0]).append(kDelimiter).append("bg");
  if (tokens.size() == 3)
    res.append(kDelimiter).append(tokens[2]);
  return res;
}

using Rings = std::vector<std::vector<m2::PointD>>;

// Removes exactly collinear vertices. Earcut can't clip them as ears and filters them only after a pass without
// ears or when bridging holes, so long runs of them, e.g. in a densified rectangle, make it quadratic. In Mercator,
// only runs along meridians and parallels stay exactly collinear, so real-life rings rarely lose a vertex here.
void RemoveCollinearPoints(std::vector<m2::PointD> & ring)
{
  size_t size = 0;
  for (auto const & point : ring)
  {
    while (size > 1 && m2::robust::OrientedS(ring[size - 2], ring[size - 1], point) == 0)
      --size;
    ring[size++] = point;
  }

  // The same across the start of the ring.
  size_t first = 0;
  while (size - first > 2)
    if (m2::robust::OrientedS(ring[size - 2], ring[size - 1], ring[first]) == 0)
      --size;
    else if (m2::robust::OrientedS(ring[size - 1], ring[first], ring[first + 1]) == 0)
      ++first;
    else
      break;
  ring.resize(size);
  ring.erase(ring.begin(), ring.begin() + first);
}

// Returns the doubled area.
double RingArea(std::vector<m2::PointD> const & ring)
{
  auto const & origin = ring.front();
  double area = 0;
  for (size_t i = 2; i < ring.size(); ++i)
    area += m2::CrossProduct(ring[i - 1] - origin, ring[i] - origin);
  return std::fabs(area);
}

// Earcut fills only valid polygons by the even-odd rule, while the import retains invalid ones,
// and the projection may make others touch or cross.
bool IsValidPolygon(Rings const & rings)
{
  namespace bg = boost::geometry;
  bg::model::polygon<bg::model::d2::point_xy<double>, false /* clockWise */, false /* closed */> polygon;
  auto const copy = [](std::vector<m2::PointD> const & from, auto & to)
  {
    to.reserve(from.size());
    for (auto const & p : from)
      to.emplace_back(p.x, p.y);
  };
  copy(rings.front(), polygon.outer());
  for (size_t i = 1; i < rings.size(); ++i)
    copy(rings[i], polygon.inners().emplace_back());
  // The even-odd rule doesn't depend on the winding of rings.
  bg::correct(polygon);
  return bg::is_valid(polygon);
}

// Fans out a triangle, a convex quadrilateral or a ring that is star-shaped around its bounding box center, as
// many simple shapes are. Earcut is slow on some of the latter, e.g. 0.6 s vs 1 ms for a jagged 100k-vertex circle.
template <class AddTriangleFn>
bool TryFan(std::vector<m2::PointD> const & ring, AddTriangleFn const & addTriangle)
{
  size_t const n = ring.size();
  // All turns of a convex quadrilateral are of the same direction, while RemoveCollinearPoints leaves no straight ones.
  auto const turnsLeft = [&](size_t i)
  { return m2::robust::OrientedS(ring[i], ring[(i + 1) % n], ring[(i + 2) % n]) > 0; };
  if (n == 3 ||
      (n == 4 && turnsLeft(0) == turnsLeft(1) && turnsLeft(1) == turnsLeft(2) && turnsLeft(2) == turnsLeft(3)))
  {
    for (size_t i = 2; i < n; ++i)
      addTriangle(ring[0], ring[i - 1], ring[i]);
    return true;
  }

  m2::RectD rect;
  for (auto const & p : ring)
    rect.Add(p);
  auto const center = rect.Center();
  // Every edge must go around the center in the same direction, by less than pi, and the ring must pass between
  // the half-planes above and below the center twice, so that it winds once. Otherwise, e.g. for a pentagram or
  // a ring traced twice, the fan triangles overlap.
  auto const isUpper = [&center](m2::PointD const & p)
  { return p.y > center.y || (p.y == center.y && p.x >= center.x); };
  bool const isLeft = m2::robust::OrientedS(center, ring.back(), ring.front()) > 0;
  size_t crossings = 0;
  for (size_t i = 0, j = n - 1; i < n; j = i++)
  {
    double const s = m2::robust::OrientedS(center, ring[j], ring[i]);
    if (s == 0 || (s > 0) != isLeft)
      return false;
    crossings += isUpper(ring[j]) != isUpper(ring[i]);
  }
  if (crossings != 2)
    return false;

  for (size_t i = 0, j = n - 1; i < n; j = i++)
    addTriangle(center, ring[j], ring[i]);
  return true;
}

template <class AddTriangleFn>
void TessellateByLibtess(Rings const & rings, AddTriangleFn const & addTriangle)
{
  std::unique_ptr<TESStesselator, decltype(&tessDeleteTess)> const tess(tessNewTess(nullptr), &tessDeleteTess);
  for (auto const & ring : rings)
    tessAddContour(tess.get(), 2, ring.data(), sizeof(m2::PointD), static_cast<int>(ring.size()));
  // The contours are planar, so libtess2 needn't find their plane and orientation.
  TESSreal constexpr normal[] = {0, 0, 1};
  // Fails only when out of memory.
  VERIFY(tessTesselate(tess.get(), TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, normal), ());

  auto const * vertices = reinterpret_cast<m2::PointD const *>(tessGetVertices(tess.get()));
  auto const * indices = tessGetElements(tess.get());
  for (int i = 0, count = tessGetElementCount(tess.get()); i < count; ++i, indices += 3)
    addTriangle(vertices[indices[0]], vertices[indices[1]], vertices[indices[2]]);
}

// By the even-odd rule, as polygons are filled.
bool IsInside(std::vector<m2::SharedSpline> const & rings, m2::PointD const & p)
{
  bool inside = false;
  for (auto const & ring : rings)
  {
    auto const & path = ring->GetPath();
    for (size_t i = 1; i < path.size(); ++i)
    {
      auto const & a = path[i - 1];
      auto const & b = path[i];
      if ((a.y > p.y) != (b.y > p.y) && p.x < a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y))
        inside = !inside;
    }
  }
  return inside;
}
}  // namespace

drape_ptr<dp::OverlayHandle> CreateUserMarkOverlayHandle(UserMarkRenderParams const & renderInfo,
                                                         TileKey const & tileKey, m2::RectD const & pixelRect)
{
  if (!renderInfo.m_isSymbolSelectable)
    return nullptr;

  dp::OverlayID overlayId(renderInfo.m_featureId, renderInfo.m_markId, tileKey.GetTileCoords(),
                          kStartUserMarkOverlayIndex + renderInfo.m_index);
  m2::PointD const pivot(renderInfo.m_pivot.x + tileKey.GetTileXOffset(), renderInfo.m_pivot.y);
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<dp::SquareHandle>(
      overlayId, renderInfo.m_anchor, pivot, pixelRect.RightTop() - pixelRect.LeftBottom(), renderInfo.m_pixelOffset,
      0 /*priority*/, true /* isBound */, renderInfo.m_minZoom, true /* isBillboard */);
  return handle;
}

void CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    MarksSource const & source, UserMarksRenderCollection const & renderParams, dp::Batcher & batcher)
{
  using UPV = UserPointVertex;
  buffer_vector<UPV, dp::Batcher::VertexPerQuad> buffer;

  m2::PointD const tileCenter = tileKey.GetWrappedDataRect().Center();

  source.ForEachMark(tileKey, renderParams, [&](UserMarkRenderParams const & renderInfo)
  {
    m2::PointF symbolSize(0.0f, 0.0f);
    dp::TextureManager::SymbolRegion symbolRegion;
    auto const symbolName = GetSymbolNameForZoomLevel(make_ref(renderInfo.m_symbolNames), tileKey);
    if (!symbolName.empty())
    {
      textures->GetSymbolRegion(symbolName, symbolRegion);
      symbolSize = symbolRegion.GetPixelSize();
    }

    m2::PointF symbolOffset = m2::PointF::Zero();
    if (renderInfo.m_symbolIsPOI)
      symbolOffset = GetSymbolOffsetForZoomLevel(make_ref(renderInfo.m_symbolOffsets), tileKey);

    if (renderInfo.m_coloredSymbols != nullptr)
    {
      GenerateColoredSymbolShapes(context, textures, renderInfo, tileKey, tileCenter, symbolOffset, symbolSize,
                                  batcher);
    }

    if (!symbolName.empty())
    {
      if (renderInfo.m_symbolIsPOI)
      {
        GeneratePoiSymbolShape(context, textures, renderInfo, tileKey, tileCenter, symbolName, symbolOffset, batcher);
      }
      else
      {
        buffer.clear();

        dp::TextureManager::SymbolRegion backgroundRegion;
        if (auto const background = GetBackgroundSymbolName(symbolName); !background.empty())
        {
          if (textures->GetSymbolRegionSafe(background, backgroundRegion))
            CHECK_EQUAL(symbolRegion.GetTextureIndex(), backgroundRegion.GetTextureIndex(), ());
        }

        m2::RectF const & texRect = symbolRegion.GetTexRect();
        m2::RectF const & bgTexRect = backgroundRegion.GetTexRect();
        m2::PointF const pxSize = symbolRegion.GetPixelSize();
        dp::Anchor const anchor = renderInfo.m_anchor;
        m2::PointD const pt = MapShape::ConvertToLocal(renderInfo.m_pivot, tileCenter, kShapeCoordScalar);
        glsl::vec3 const pos = glsl::vec3(glsl::ToVec2(pt), renderInfo.m_depth);
        bool const runAnim = renderInfo.m_hasCreationAnimation && renderInfo.m_justCreated;

        glsl::vec2 left, right, up, down;
        AlignHorizontal(pxSize.x * 0.5f, anchor, left, right);
        AlignVertical(pxSize.y * 0.5f, anchor, up, down);

        m2::PointD const pixelOffset = renderInfo.m_pixelOffset;
        glsl::vec2 const offset(pixelOffset.x, pixelOffset.y);

        dp::Color color = dp::Color::White();
        if (renderInfo.m_customColor)  // explicit custom color
          color = *renderInfo.m_customColor;
        else if (!renderInfo.m_color.empty())  // preset, theme-aware
          color = df::GetColorConstant(renderInfo.m_color);

        glsl::vec4 maskColor(color.GetRedF(), color.GetGreenF(), color.GetBlueF(), renderInfo.m_symbolOpacity);
        float animateOrZ = 0.0f;
        if (!renderInfo.m_customDepth)
          animateOrZ = runAnim ? 1.0f : -1.0f;

        buffer.emplace_back(pos, glsl::vec3(left + down + offset, animateOrZ),
                            glsl::ToVec4(m2::PointD(texRect.LeftTop()), m2::PointD(bgTexRect.LeftTop())), maskColor);
        buffer.emplace_back(pos, glsl::vec3(left + up + offset, animateOrZ),
                            glsl::ToVec4(m2::PointD(texRect.LeftBottom()), m2::PointD(bgTexRect.LeftBottom())),
                            maskColor);
        buffer.emplace_back(pos, glsl::vec3(right + down + offset, animateOrZ),
                            glsl::ToVec4(m2::PointD(texRect.RightTop()), m2::PointD(bgTexRect.RightTop())), maskColor);
        buffer.emplace_back(pos, glsl::vec3(right + up + offset, animateOrZ),
                            glsl::ToVec4(m2::PointD(texRect.RightBottom()), m2::PointD(bgTexRect.RightBottom())),
                            maskColor);

        m2::RectD rect;
        for (auto const & vertex : buffer)
          rect.Add(glsl::FromVec2(glsl::vec2(vertex.m_normalAndAnimateOrZ)));

        drape_ptr<dp::OverlayHandle> overlayHandle = CreateUserMarkOverlayHandle(renderInfo, tileKey, rect);

        gpu::Program program;
        gpu::Program program3d;
        if (renderInfo.m_isMarkAboveText)
        {
          program = runAnim ? gpu::Program::BookmarkAnimAboveText : gpu::Program::BookmarkAboveText;
          program3d = runAnim ? gpu::Program::BookmarkAnimAboveTextBillboard : gpu::Program::BookmarkAboveTextBillboard;
        }
        else
        {
          program = runAnim ? gpu::Program::BookmarkAnim : gpu::Program::Bookmark;
          program3d = runAnim ? gpu::Program::BookmarkAnimBillboard : gpu::Program::BookmarkBillboard;
        }
        auto state = CreateRenderState(program, renderInfo.m_depthLayer);
        state.SetProgram3d(program3d);
        state.SetColorTexture(symbolRegion.GetTexture());
        state.SetTextureFilter(dp::TextureFilter::Nearest);
        state.SetDepthTestEnabled(renderInfo.m_depthTestEnabled);
        state.SetTextureIndex(symbolRegion.GetTextureIndex());

        dp::AttributeProvider attribProvider(1, static_cast<uint32_t>(buffer.size()));
        attribProvider.InitStream(0, UPV::GetBinding(), make_ref(buffer.data()));

        batcher.InsertListOfStrip(context, state, make_ref(&attribProvider), std::move(overlayHandle),
                                  dp::Batcher::VertexPerQuad);
      }
    }

    if (renderInfo.m_titleDecl != nullptr)
      GenerateTextShapes(context, textures, renderInfo, tileKey, tileCenter, symbolOffset, symbolSize, batcher);

    renderInfo.m_justCreated = false;
  });
}

void CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    TracksSource const & source, UserLinesRenderCollection const & renderParams, dp::Batcher & batcher)
{
  CHECK_GREATER(tileKey.m_zoomLevel, 0, ());
  CHECK_LESS(tileKey.m_zoomLevel - 1, static_cast<int>(kLineWidthZoomFactor.size()), ());

  double const vs = df::VisualParams::Instance().GetVisualScale();
  bool const simplify = tileKey.m_zoomLevel <= 15;

  // This var is used only if simplify == true.
  double minSegmentSqrLength = 1.0;
  if (simplify)
    minSegmentSqrLength = math::Pow2(4.0 * vs * GetScreenScale(tileKey.m_zoomLevel));

  m2::RectD const tileRect = tileKey.GetWrappedDataRect();
  bool const showDirection = tileKey.m_zoomLevel >= kTrackDirectionMinZoom;
  double const pixelToGlobalScale = GetScreenScale(tileKey.m_zoomLevel);
  float const arrowOffset = kTrackArrowOffset * static_cast<float>(vs);
  float const arrowStep = kTrackArrowStep * static_cast<float>(vs);

  auto endpointSearchRect = tileRect;
  if (showDirection)
  {
    dp::TextureManager::SymbolRegion region;
    textures->GetSymbolRegion(kTrackDirectionBlackSymbol, region);
    // Include neighboring markers that can overlap even the largest padded, rotated arrow quad.
    double const maxArrowExtent =
        0.5 * region.GetPixelSize().Length() * (kTrackArrowMaxHeight / kTrackArrowHeight) * pixelToGlobalScale;
    endpointSearchRect.Inflate(maxArrowExtent, maxArrowExtent);
  }

  // Arrows render after all endpoint markers, so every track must keep clear of every marker.
  std::vector<m2::RectD> endpointRects;
  if (tileKey.m_zoomLevel >= kTrackEndpointMinZoom)
  {
    double const radius = (kTrackEndpointRadius + kTrackEndpointOutlineWidth) * vs * pixelToGlobalScale;
    source.ForEachUniqueTrack(tileKey.m_zoomLevel, renderParams, [&](UserLineRenderParams const & renderInfo)
    {
      if (!renderInfo.m_hasDirection || renderInfo.m_layers.empty() || renderInfo.m_splines.empty())
        return;

      auto const & topLayer = GetTopLayer(renderInfo);
      float const depth = topLayer.m_depth + 2.0f * kTrackShapeDepthOffset;
      auto const & start = renderInfo.m_splines.front()->GetPath().front();
      auto const & finish = renderInfo.m_splines.back()->GetPath().back();
      bool const combined =
          start.SquaredLength(finish) <= math::Pow2(kTrackCombinedEndpointDistance * vs * pixelToGlobalScale);
      auto const cacheEndpoint = [&](m2::PointD const & point, TrackEndpoint endpoint)
      {
        m2::RectD const rect(point, radius, radius);
        if (showDirection && endpointSearchRect.IsIntersect(rect))
          endpointRects.push_back(rect);
        if (tileRect.IsPointInside(point))
          DrawTrackEndpoint(context, tileKey, textures, point, endpoint, topLayer.m_color, depth,
                            renderInfo.m_depthLayer, static_cast<float>(vs), batcher);
      };
      cacheEndpoint(start, combined ? TrackEndpoint::StartAndFinish : TrackEndpoint::Start);
      if (!combined)
        cacheEndpoint(finish, TrackEndpoint::Finish);
    });
  }

  source.ForEachUniqueTrack(tileKey.m_zoomLevel, renderParams, [&](UserLineRenderParams const & renderInfo)
  {
    if (renderInfo.m_layers.empty() || renderInfo.m_splines.empty())
      return;

    auto const & topLayer = GetTopLayer(renderInfo);
    float const shapesDepth = topLayer.m_depth + kTrackShapeDepthOffset;
    float const trackWidth =
        topLayer.m_width * static_cast<float>(vs) * static_cast<float>(kLineWidthZoomFactor[tileKey.m_zoomLevel - 1]);
    float const maxArrowHeight = std::min(trackWidth - 2.0f * kTrackArrowInset * static_cast<float>(vs),
                                          kTrackArrowMaxHeight * static_cast<float>(vs));
    float const arrowScale = std::max(0.0f, maxArrowHeight / (kTrackArrowHeight * static_cast<float>(vs)));
    bool const drawDirection = showDirection && renderInfo.m_hasDirection && arrowScale >= kTrackArrowMinScale;
    float const arrowHeight = kTrackArrowHeight * static_cast<float>(vs) * arrowScale;
    // The size calculation already reserves the inset on both sides. That reserved space is the curvature budget.
    float const maxPathDeviation = std::max(0.0f, 0.5f * (trackWidth - arrowHeight));
    char const * directionSymbol = nullptr;
    if (drawDirection)
      directionSymbol = GetContrastingColor(topLayer.m_color) == dp::Color::Black() ? kTrackDirectionBlackSymbol
                                                                                    : kTrackDirectionWhiteSymbol;

    // Spline is a shared_ptr here, can reassign later.
    for (auto spline : renderInfo.m_splines)
    {
      if (simplify)
        spline = SimplifySpline(spline, minSegmentSqrLength);

      if (spline->GetSize() < 2)
        continue;

      std::vector<double> sourceOffsets;
      auto const clippedSplines = drawDirection ? m2::ClipSplineByRect(tileRect, spline, sourceOffsets)
                                                : m2::ClipSplineByRect(tileRect, spline);
      for (size_t clippedIndex = 0; clippedIndex < clippedSplines.size(); ++clippedIndex)
      {
        auto const & clippedSpline = clippedSplines[clippedIndex];
        for (auto const & layer : renderInfo.m_layers)
        {
          LineViewParams params;
          params.m_tileCenter = tileRect.Center();
          params.m_baseGtoPScale = 1.0f;
          params.m_cap = dp::RoundCap;
          params.m_join = dp::RoundJoin;
          params.m_color = layer.m_color;
          params.m_depthTestEnabled = true;
          params.m_depth = layer.m_depth;
          params.m_depthLayer = renderInfo.m_depthLayer;
          params.m_width = static_cast<float>(layer.m_width * vs * kLineWidthZoomFactor[tileKey.m_zoomLevel - 1]);
          params.m_minVisibleScale = 1;
          params.m_rank = 0;

          LineShape(clippedSpline, params).Draw(context, make_ref(&batcher), textures);
        }

        if (drawDirection)
        {
          PathSymbolViewParams params;
          params.m_tileCenter = tileRect.Center();
          params.m_depthTestEnabled = false;
          params.m_depth = shapesDepth;
          params.m_depthLayer = renderInfo.m_depthLayer;
          params.m_symbolName = directionSymbol;
          params.m_offset = GetNextArrowOffset(sourceOffsets[clippedIndex], pixelToGlobalScale, arrowOffset, arrowStep);
          params.m_step = arrowStep;
          params.m_symbolScale = arrowScale;
          params.m_placementLength = kTrackArrowLength * static_cast<float>(vs) * arrowScale;
          params.m_maxPathDeviation = maxPathDeviation;
          params.m_searchRadius = kTrackArrowSearchRadius * static_cast<float>(vs);
          params.m_exclusionRects = endpointRects;
          params.m_baseGtoPScale = 1.0 / pixelToGlobalScale;
          PathSymbolShape(clippedSpline, params).Draw(context, make_ref(&batcher), textures);
        }
      }
    }
  });
}

drape_ptr<UserAreaRenderParams> GenerateAreaRenderInfo(kml::AreaData const & area)
{
  auto renderInfo = make_unique_dp<UserAreaRenderParams>();
  if (!area.m_visible)
    return renderInfo;

  auto const & style = area.m_style;
  if (style.m_fill)
    renderInfo->m_fillColor = dp::Color(style.m_fillColor);
  renderInfo->m_outlineColor = dp::Color(style.m_outlineColor);
  if (style.m_outline && style.m_outlineWidth > 0 && renderInfo->m_outlineColor.GetAlpha() > 0)
    renderInfo->m_outlineWidth = static_cast<float>(style.m_outlineWidth);

  auto const toMercator = [](kml::AreaRing const & ring)
  {
    ASSERT_GREATER_OR_EQUAL(ring.size(), 3, ());
    std::vector<m2::PointD> points;
    points.reserve(ring.size());
    // Without repeats of vertices that differ only in height, also across the start of the ring.
    for (auto const & point : ring)
      if (auto const p = mercator::FromLatLon(point.m_latLon); points.empty() || p != points.back())
        points.push_back(p);
    if (points.size() > 1 && points.back() == points.front())
      points.pop_back();
    return points;
  };
  auto const makeSpline = [](std::vector<m2::PointD> const & ring)
  {
    auto const closing = (ring.front() + ring.back()) / 2;
    auto spline = std::make_unique<m2::Spline>(ring.size() + 2);
    spline->AddPoint(closing);
    // Skips segments shorter than the spline's minimum, so a tiny ring collapses into a point.
    for (auto const & point : ring)
      spline->AddPoint(point);
    spline->AddPoint(closing);
    return m2::SharedSpline(std::move(spline));
  };

  // Global and polar footprints need a separate wrapping/clipping implementation.
  double const maxLat = mercator::YToLat(mercator::Bounds::kMaxY);
  for (auto const & polygon : area.m_polygons)
  {
    // Holes may lie outside their shell, and the even-odd rule fills them.
    std::vector<kml::AreaRing const *> rings = {&polygon.m_outer};
    for (auto const & hole : polygon.m_holes)
      rings.push_back(&hole);

    m2::RectD geoRect;
    for (auto const * ring : rings)
      for (auto const & point : *ring)
        geoRect.Add({point.m_latLon.m_lon, point.m_latLon.m_lat});
    if (geoRect.SizeX() > 180 || geoRect.minY() < -maxLat || geoRect.maxY() > maxLat)
    {
      LOG(LWARNING, ("Area polygon lies outside the supported display extent", area.m_id));
      continue;
    }

    auto & splines = renderInfo->m_polygons.emplace_back();
    auto & rect = renderInfo->m_polygonRects.emplace_back();
    std::vector<std::vector<m2::PointD>> fillRings;
    for (auto const * ring : rings)
    {
      auto const & points = fillRings.emplace_back(toMercator(*ring));
      splines.push_back(makeSpline(points));
      for (auto const & point : points)
        rect.Add(point);
    }
    renderInfo->m_limitRect.Add(rect);
    if (renderInfo->m_fillColor.GetAlpha() > 0)
    {
      renderInfo->m_fillRings.push_back(std::move(fillRings));
      renderInfo->m_triangles.emplace_back();
    }
  }
  return renderInfo;
}

void TessellateUserArea(UserAreaRenderParams & area, size_t polygon)
{
  ASSERT_LESS(polygon, area.m_fillRings.size(), ());
  auto rings = std::exchange(area.m_fillRings[polygon], {});
  // Tessellated already.
  if (rings.empty())
    return;

  auto & triangles = area.m_triangles[polygon];
  // Returns the doubled area of the triangle.
  auto const addTriangle = [&triangles](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
  {
    // ClipTriangleByRect takes clockwise triangles, while tessellators may emit others and zero-area ones.
    double const s = m2::robust::OrientedS(a, b, c);
    if (s < 0)
      triangles.insert(triangles.end(), {a, b, c});
    else if (s > 0)
      triangles.insert(triangles.end(), {a, c, b});
    return std::fabs(s);
  };

  for (auto & ring : rings)
    RemoveCollinearPoints(ring);
  if (rings.front().size() < 3)
    return;
  std::erase_if(rings, [](auto const & ring) { return ring.size() < 3; });
  if (rings.size() == 1 && TryFan(rings.front(), addTriangle))
    return;

  // Even with the validity check, earcut is 1.4-11 times faster than libtess2 on real-life areas, e.g. 0.13 s vs
  // 1.5 s for the Natural England national parks (10 polygons, 490k vertices, arm64 -O2). But it is slower at
  // bridging many holes, e.g. 18 ms vs 1.5 ms for 1024 ones.
  size_t constexpr kMaxEarcutRings = 128;
  if (rings.size() <= kMaxEarcutRings && IsValidPolygon(rings))
  {
    std::vector<m2::PointD> points;
    double expectedArea = 0;
    for (size_t i = 0; i < rings.size(); ++i)
    {
      points.insert(points.end(), rings[i].begin(), rings[i].end());
      expectedArea += i == 0 ? RingArea(rings[i]) : -RingArea(rings[i]);
    }

    double earcutArea = 0;
    auto const indices = mapbox::earcut<uint32_t>(rings);
    for (size_t i = 0; i < indices.size(); i += 3)
      earcutArea += addTriangle(points[indices[i]], points[indices[i + 1]], points[indices[i + 2]]);
    // Guards against earcut failures on valid polygons, like its -ffast-math miscompilation of infinity.
    if (std::fabs(earcutArea - expectedArea) <= 1e-6 * expectedArea)
      return;
    triangles.clear();
  }
  // libtess2 fills any polygon by the even-odd rule, which the import keeps for invalid ones.
  TessellateByLibtess(rings, addTriangle);
}

void CacheUserAreas(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    UserAreasToCache const & areas, dp::Batcher & batcher)
{
  double const vs = VisualParams::Instance().GetVisualScale();
  bool const simplify = tileKey.m_zoomLevel <= 15;
  double const minSegmentSqrLength = math::Pow2(4.0 * vs * GetScreenScale(tileKey.m_zoomLevel));
  m2::RectD const tileRect = tileKey.GetWrappedDataRect();

  for (auto const & [area, depth] : areas)
  {
    auto const inTile = [&](size_t polygon) { return area->m_polygonRects[polygon].IsIntersect(tileRect); };

    std::vector<m2::PointD> triangles;
    for (size_t p = 0; p < area->m_triangles.size(); ++p)
    {
      if (!inTile(p))
        continue;
      // A tile without rings is all inside the polygon or all outside, so it needs no triangles, which may split
      // the tile into many pieces, e.g. around the center of a fan.
      auto const & rings = area->m_polygons[p];
      if (std::ranges::none_of(rings, [&tileRect](auto const & ring) { return m2::IsRealIntersect(tileRect, *ring); }))
      {
        if (IsInside(rings, tileRect.Center()))
        {
          triangles.insert(triangles.end(), {tileRect.LeftBottom(), tileRect.LeftTop(), tileRect.RightTop(),
                                             tileRect.LeftBottom(), tileRect.RightTop(), tileRect.RightBottom()});
        }
        continue;
      }
      TessellateUserArea(*area, p);
      auto const & t = area->m_triangles[p];
      for (size_t i = 0; i < t.size(); i += 3)
      {
        m2::ClipTriangleByRect(tileRect, t[i], t[i + 1], t[i + 2],
                               [&triangles](auto const & a, auto const & b, auto const & c)
        { triangles.insert(triangles.end(), {a, b, c}); });
      }
    }
    if (!triangles.empty())
    {
      // Fills keep the default 0 depth, so they never cover any outline.
      AreaViewParams params;
      params.m_tileCenter = tileRect.Center();
      params.m_depthLayer = DepthLayer::UserAreaLayer;
      params.m_color = area->m_fillColor;
      params.m_blendInPaintOrder = true;
      AreaShape(std::move(triangles), {}, params).Draw(context, make_ref(&batcher), textures);
    }

    if (area->m_outlineWidth == 0)
      continue;

    LineViewParams params;
    params.m_tileCenter = tileRect.Center();
    params.m_depthLayer = DepthLayer::UserAreaLayer;
    params.m_depth = depth;
    params.m_depthFunction = dp::TestFunction::Less;
    params.m_blendInPaintOrder = true;
    params.m_width = static_cast<float>(area->m_outlineWidth * vs);
    params.m_color = area->m_outlineColor;
    params.m_cap = dp::ButtCap;
    params.m_join = dp::RoundJoin;
    params.m_zoomLevel = tileKey.m_zoomLevel;
    for (size_t p = 0; p < area->m_polygons.size(); ++p)
    {
      if (!inTile(p))
        continue;
      for (auto ring : area->m_polygons[p])
      {
        if (simplify)
          ring = SimplifyRing(ring, minSegmentSqrLength);
        if (ring->GetSize() < 2)
          continue;
        // An outline along a tile side is cached by both tiles, but its second copy fails the depth test.
        for (auto const & spline : m2::ClipSplineByRect(tileRect, ring))
          LineShape(spline, params).Draw(context, make_ref(&batcher), textures);
      }
    }
  }
}
}  // namespace df
