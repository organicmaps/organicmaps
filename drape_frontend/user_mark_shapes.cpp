#include "drape_frontend/user_mark_shapes.hpp"

#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/scales.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"

#include <cmath>
#include <vector>

namespace df
{
std::vector<double> const kLineWidthZoomFactor =
{
// 1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16   17   18   19
  0.3, 0.3, 0.3, 0.4, 0.5, 0.6, 0.7, 0.7, 0.7, 0.7, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0
};
int const kLineSimplifyLevelEnd = 15;

namespace
{
template <typename TCreateVector>
void AlignFormingNormals(TCreateVector const & fn, dp::Anchor anchor, dp::Anchor first,
                         dp::Anchor second, glsl::vec2 & firstNormal, glsl::vec2 & secondNormal)
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
  AlignFormingNormals([&halfWidth] { return glsl::vec2(-halfWidth, 0.0f); }, anchor, dp::Left,
                      dp::Right, left, right);
}

void AlignVertical(float halfHeight, dp::Anchor anchor, glsl::vec2 & up, glsl::vec2 & down)
{
  AlignFormingNormals([&halfHeight] { return glsl::vec2(0.0f, -halfHeight); }, anchor, dp::Top,
                      dp::Bottom, up, down);
}

struct UserPointVertex : public gpu::BaseVertex
{
  using TTexCoord = glsl::vec4;
  using TColorAndAnimate = glsl::vec4;

  UserPointVertex() = default;
  UserPointVertex(TPosition const & pos, TNormal const & normal, TTexCoord const & texCoord,
                  TColorAndAnimate const & colorAndAnimate)
    : m_position(pos), m_normal(normal), m_texCoord(texCoord), m_colorAndAnimate(colorAndAnimate)
  {}

  static dp::BindingInfo GetBinding()
  {
    dp::BindingInfo info(4);
    uint8_t offset = 0;
    offset += dp::FillDecl<TPosition, UserPointVertex>(0, "a_position", info, offset);
    offset += dp::FillDecl<TNormal, UserPointVertex>(1, "a_normal", info, offset);
    offset += dp::FillDecl<TTexCoord, UserPointVertex>(2, "a_texCoords", info, offset);
    offset += dp::FillDecl<TColorAndAnimate, UserPointVertex>(3, "a_colorAndAnimate", info, offset);

    return info;
  }

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_texCoord;
  TColorAndAnimate m_colorAndAnimate;
};

std::string GetSymbolNameForZoomLevel(drape_ptr<UserPointMark::SymbolNameZoomInfo> const & symbolNames,
                                      TileKey const & tileKey)
{
  if (!symbolNames)
    return {};

  for (auto itName = symbolNames->crbegin(); itName != symbolNames->crend(); ++itName)
  {
    if (itName->first <= tileKey.m_zoomLevel)
      return itName->second;
  }
  return {};
}

void GenerateColoredSymbolShapes(ref_ptr<dp::GraphicsContext> context,
                                 UserMarkRenderParams const & renderInfo, TileKey const & tileKey,
                                 m2::PointD const & tileCenter, ref_ptr<dp::TextureManager> textures,
                                 m2::PointF & symbolSize, dp::Batcher & batcher)
{
  m2::PointF sizeInc(0.0f, 0.0f);
  m2::PointF offset(0.0f, 0.0f);
  UserPointMark::SymbolSizes symbolSizesInc;

  auto const isTextBg = renderInfo.m_coloredSymbols->m_addTextSize;

  if (isTextBg)
  {
    auto const & titleDecl = renderInfo.m_titleDecl->at(0);
    dp::FontDecl const & fontDecl = titleDecl.m_primaryTextFont;
    auto isSdf = df::VisualParams::Instance().IsSdfPrefered();
    isSdf = fontDecl.m_outlineColor != dp::Color::Transparent() ? true : isSdf;
    auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

    TextLayout textLayout;
    textLayout.Init(strings::MakeUniString(titleDecl.m_primaryText), fontDecl.m_size * vs, isSdf, textures);
    sizeInc.x = textLayout.GetPixelLength();
    sizeInc.y = textLayout.GetPixelHeight();

    if (renderInfo.m_symbolSizes != nullptr)
    {
      symbolSizesInc.reserve(renderInfo.m_symbolSizes->size());
      for (auto const & sz : *renderInfo.m_symbolSizes)
        symbolSizesInc.push_back(sz + sizeInc);
    }

    offset = StraightTextLayout::GetSymbolBasedTextOffset(symbolSize, titleDecl.m_anchor, renderInfo.m_anchor);
  }

  for (auto itSym = renderInfo.m_coloredSymbols->m_zoomInfo.rbegin();
       itSym != renderInfo.m_coloredSymbols->m_zoomInfo.rend(); ++itSym)
  {
    if (itSym->first <= tileKey.m_zoomLevel)
    {
      ColoredSymbolViewParams params = itSym->second;

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

      params.m_featureID = renderInfo.m_featureId;
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
        ColoredSymbolShape(renderInfo.m_pivot, params, tileKey,
                           kStartUserMarkOverlayIndex + renderInfo.m_index,
                           isTextBg ? symbolSizesInc : *renderInfo.m_symbolSizes.get())
            .Draw(context, &batcher, textures);
      }
      else
      {
        ColoredSymbolShape(renderInfo.m_pivot, params, tileKey,
                           kStartUserMarkOverlayIndex + renderInfo.m_index, renderInfo.m_coloredSymbols->m_needOverlay)
            .Draw(context, &batcher, textures);
      }
      break;
    }
  }
}

void GeneratePoiSymbolShape(ref_ptr<dp::GraphicsContext> context,
                            UserMarkRenderParams const & renderInfo, TileKey const & tileKey,
                            m2::PointD const & tileCenter, std::string const & symbolName,
                            ref_ptr<dp::TextureManager> textures, m2::PointF & symbolOffset,
                            dp::Batcher & batcher)
{
  PoiSymbolViewParams params(renderInfo.m_featureId);
  params.m_tileCenter = tileCenter;
  params.m_depthTestEnabled = renderInfo.m_depthTestEnabled;
  params.m_depth = renderInfo.m_depth;
  params.m_depthLayer = renderInfo.m_depthLayer;
  params.m_minVisibleScale = renderInfo.m_minZoom;
  params.m_specialDisplacement = renderInfo.m_displacement;
  params.m_specialPriority = renderInfo.m_priority;
  params.m_symbolName = symbolName;
  params.m_anchor = renderInfo.m_anchor;

  bool const hasColoredOverlay = renderInfo.m_coloredSymbols != nullptr && renderInfo.m_coloredSymbols->m_needOverlay;
  params.m_startOverlayRank = hasColoredOverlay ? dp::OverlayRank1 : dp::OverlayRank0;

  if (renderInfo.m_symbolOffsets != nullptr)
  {
    ASSERT_GREATER(tileKey.m_zoomLevel, 0, ());
    ASSERT_LESS_OR_EQUAL(tileKey.m_zoomLevel, scales::UPPER_STYLE_SCALE, ());
    size_t offsetIndex = 0;
    if (tileKey.m_zoomLevel > 0)
      offsetIndex = static_cast<size_t>(std::min(tileKey.m_zoomLevel, scales::UPPER_STYLE_SCALE) - 1);
    symbolOffset = renderInfo.m_symbolOffsets->at(offsetIndex);
    params.m_offset = symbolOffset;
  }
  PoiSymbolShape(renderInfo.m_pivot, params, tileKey,
                 kStartUserMarkOverlayIndex + renderInfo.m_index)
      .Draw(context, &batcher, textures);
}

void GenerateTextShapes(ref_ptr<dp::GraphicsContext> context,
                        UserMarkRenderParams const & renderInfo, TileKey const & tileKey,
                        m2::PointD const & tileCenter, m2::PointF const & symbolSize,
                        m2::PointF const & symbolOffset, ref_ptr<dp::TextureManager> textures,
                        dp::Batcher & batcher)
{
  if (renderInfo.m_minTitleZoom > tileKey.m_zoomLevel)
    return;

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  for (auto const & titleDecl : *renderInfo.m_titleDecl)
  {
    if (titleDecl.m_primaryText.empty())
      continue;

    TextViewParams params;
    params.m_featureID = renderInfo.m_featureId;
    params.m_tileCenter = tileCenter;
    params.m_titleDecl = titleDecl;

    // Here we use visual scale to adapt texts sizes and offsets
    // to different screen resolutions and DPI.
    params.m_titleDecl.m_primaryTextFont.m_size *= vs;
    params.m_titleDecl.m_secondaryTextFont.m_size *= vs;
    params.m_titleDecl.m_primaryOffset *= vs;
    params.m_titleDecl.m_secondaryOffset *= vs;
    bool const isSdf = df::VisualParams::Instance().IsSdfPrefered();
    params.m_titleDecl.m_primaryTextFont.m_isSdf =
      params.m_titleDecl.m_primaryTextFont.m_outlineColor != dp::Color::Transparent() ? true : isSdf;
    params.m_titleDecl.m_secondaryTextFont.m_isSdf =
      params.m_titleDecl.m_secondaryTextFont.m_outlineColor != dp::Color::Transparent() ? true : isSdf;

    params.m_depthTestEnabled = renderInfo.m_depthTestEnabled;
    params.m_depth = renderInfo.m_depth;
    params.m_depthLayer = renderInfo.m_depthLayer;
    params.m_minVisibleScale = renderInfo.m_minZoom;

    uint32_t const overlayIndex = kStartUserMarkOverlayIndex + renderInfo.m_index;
    if (renderInfo.m_hasTitlePriority)
    {
      params.m_specialDisplacement = renderInfo.m_displacement;
      params.m_specialPriority = renderInfo.m_priority;
      params.m_startOverlayRank = dp::OverlayRank0;
      if (renderInfo.m_hasSymbolShapes)
      {
        if (renderInfo.m_symbolNames != nullptr)
          params.m_startOverlayRank++;
        if (renderInfo.m_coloredSymbols != nullptr && renderInfo.m_coloredSymbols->m_needOverlay)
          params.m_startOverlayRank++;
        ASSERT_LESS(params.m_startOverlayRank, dp::OverlayRanksCount, ());
      }
    }

    if (renderInfo.m_symbolSizes != nullptr)
    {
      TextShape(renderInfo.m_pivot, params, tileKey, *renderInfo.m_symbolSizes,
                m2::PointF(0.0f, 0.0f) /* symbolOffset */, renderInfo.m_anchor, overlayIndex)
          .Draw(context, &batcher, textures);
    }
    else
    {
      TextShape(renderInfo.m_pivot, params, tileKey, symbolSize, symbolOffset, renderInfo.m_anchor,
                overlayIndex)
          .Draw(context, &batcher, textures);
    }
  }
}

m2::SharedSpline SimplifySpline(UserLineRenderParams const & renderInfo, double sqrScale)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  m2::SharedSpline spline;
  spline.Reset(new m2::Spline(renderInfo.m_spline->GetSize()));

  static double const kMinSegmentLength = std::pow(4.0 * vs, 2);
  m2::PointD lastAddedPoint;
  for (auto const & point : renderInfo.m_spline->GetPath())
  {
    if (spline->GetSize() > 1 && point.SquaredLength(lastAddedPoint) * sqrScale < kMinSegmentLength)
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

std::string GetBackgroundForSymbol(std::string const & symbolName,
                                   ref_ptr<dp::TextureManager> textures)
{
  static std::string const kDelimiter = "-";
  static std::string const kBackgroundName = "bg";
  auto const tokens = strings::Tokenize(symbolName, kDelimiter.c_str());
  if (tokens.size() < 2 || tokens.size() > 3)
    return {};
  std::string backgroundSymbol;
  if (tokens.size() == 2)
    backgroundSymbol = tokens[0] + kDelimiter + kBackgroundName;
  else
    backgroundSymbol = tokens[0] + kDelimiter + kBackgroundName + kDelimiter + tokens[2];
  return textures->HasSymbolRegion(backgroundSymbol) ? backgroundSymbol : "";
}
}  // namespace

void CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                    ref_ptr<dp::TextureManager> textures, kml::MarkIdCollection const & marksId,
                    UserMarksRenderCollection & renderParams, dp::Batcher & batcher)
{
  using UPV = UserPointVertex;
  buffer_vector<UPV, dp::Batcher::VertexPerQuad> buffer;

  for (auto const id : marksId)
  {
    auto const it = renderParams.find(id);
    if (it == renderParams.end())
      continue;

    UserMarkRenderParams & renderInfo = *it->second;
    if (!renderInfo.m_isVisible)
      continue;

    m2::PointD const tileCenter = tileKey.GetGlobalRect().Center();

    m2::PointF symbolSize(0.0f, 0.0f);
    m2::PointF symbolOffset(0.0f, 0.0f);

    auto const symbolName = GetSymbolNameForZoomLevel(renderInfo.m_symbolNames, tileKey);
    if (!symbolName.empty())
    {
      dp::TextureManager::SymbolRegion region;
      textures->GetSymbolRegion(symbolName, region);
      symbolSize = region.GetPixelSize();
    }

    dp::Color color = dp::Color::White();
    if (!renderInfo.m_color.empty())
      color = df::GetColorConstant(renderInfo.m_color);

    if (renderInfo.m_hasSymbolShapes)
    {
      if (renderInfo.m_coloredSymbols != nullptr)
      {
        GenerateColoredSymbolShapes(context, renderInfo, tileKey, tileCenter, textures, symbolSize,
                                    batcher);
      }

      if (renderInfo.m_symbolNames != nullptr)
      {
        GeneratePoiSymbolShape(context, renderInfo, tileKey, tileCenter, symbolName, textures,
                               symbolOffset, batcher);
      }
    }
    else if (renderInfo.m_symbolNames != nullptr)
    {
      dp::TextureManager::SymbolRegion region;
      dp::TextureManager::SymbolRegion backgroundRegion;

      buffer.clear();
      textures->GetSymbolRegion(symbolName, region);
      auto const backgroundSymbol = GetBackgroundForSymbol(symbolName, textures);
      if (!backgroundSymbol.empty())
        textures->GetSymbolRegion(backgroundSymbol, backgroundRegion);

      m2::RectF const & texRect = region.GetTexRect();
      m2::RectF const & bgTexRect = backgroundRegion.GetTexRect();
      m2::PointF const pxSize = region.GetPixelSize();
      dp::Anchor const anchor = renderInfo.m_anchor;
      m2::PointD const pt = MapShape::ConvertToLocal(renderInfo.m_pivot, tileCenter,
                                                     kShapeCoordScalar);
      glsl::vec3 const pos = glsl::vec3(glsl::ToVec2(pt), renderInfo.m_depth);
      bool const runAnim = renderInfo.m_hasCreationAnimation && renderInfo.m_justCreated;

      glsl::vec2 left, right, up, down;
      AlignHorizontal(pxSize.x * 0.5f, anchor, left, right);
      AlignVertical(pxSize.y * 0.5f, anchor, up, down);

      m2::PointD const pixelOffset = renderInfo.m_pixelOffset;
      glsl::vec2 const offset(pixelOffset.x, pixelOffset.y);
      up += offset;
      down += offset;

      glsl::vec4 colorAndAnimate(color.GetRedF(), color.GetGreenF(), color.GetBlueF(),
                                 runAnim ? 1.0f : -1.0f);
      buffer.emplace_back(pos, left + down,
                          glsl::ToVec4(m2::PointD(texRect.LeftTop()), m2::PointD(bgTexRect.LeftTop())),
                          colorAndAnimate);
      buffer.emplace_back(pos, left + up,
                          glsl::ToVec4(m2::PointD(texRect.LeftBottom()), m2::PointD(bgTexRect.LeftBottom())),
                          colorAndAnimate);
      buffer.emplace_back(pos, right + down,
                          glsl::ToVec4(m2::PointD(texRect.RightTop()), m2::PointD(bgTexRect.RightTop())),
                          colorAndAnimate);
      buffer.emplace_back(pos, right + up,
                          glsl::ToVec4(m2::PointD(texRect.RightBottom()), m2::PointD(bgTexRect.RightBottom())),
                          colorAndAnimate);

      gpu::Program program;
      gpu::Program program3d;
      if (renderInfo.m_isMarkAboveText)
      {
        program = runAnim ? gpu::Program::BookmarkAnimAboveText
                          : gpu::Program::BookmarkAboveText;
        program3d = runAnim ? gpu::Program::BookmarkAnimAboveTextBillboard
                            : gpu::Program::BookmarkAboveTextBillboard;
      }
      else
      {
        program = runAnim ? gpu::Program::BookmarkAnim
                          : gpu::Program::Bookmark;
        program3d = runAnim ? gpu::Program::BookmarkAnimBillboard
                            : gpu::Program::BookmarkBillboard;
      }
      auto state = CreateRenderState(program, renderInfo.m_depthLayer);
      state.SetProgram3d(program3d);
      state.SetColorTexture(region.GetTexture());
      state.SetTextureFilter(dp::TextureFilter::Nearest);
      state.SetDepthTestEnabled(renderInfo.m_depthTestEnabled);

      dp::AttributeProvider attribProvider(1, static_cast<uint32_t>(buffer.size()));
      attribProvider.InitStream(0, UPV::GetBinding(), make_ref(buffer.data()));

      batcher.InsertListOfStrip(context, state, make_ref(&attribProvider), dp::Batcher::VertexPerQuad);
    }

    if (renderInfo.m_titleDecl != nullptr)
    {
      GenerateTextShapes(context, renderInfo, tileKey, tileCenter, symbolSize, symbolOffset,
                         textures, batcher);
    }

    if (renderInfo.m_badgeNames != nullptr)
    {
      ASSERT(!renderInfo.m_hasSymbolShapes || renderInfo.m_symbolNames == nullptr,
             ("Multiple POI shapes in an usermark are not supported yet"));
      auto const badgeName = GetSymbolNameForZoomLevel(renderInfo.m_badgeNames, tileKey);
      if (!badgeName.empty())
      {
        GeneratePoiSymbolShape(context, renderInfo, tileKey, tileCenter, badgeName, textures,
                               symbolOffset, batcher);
      }
    }

    renderInfo.m_justCreated = false;
  }
}

void ProcessSplineSegmentRects(m2::SharedSpline const & spline, double maxSegmentLength,
                               const std::function<bool(const m2::RectD & segmentRect)> & func)
{
  double const splineFullLength = spline->GetLength();
  double length = 0;
  while (length < splineFullLength)
  {
    m2::RectD splineRect;

    auto const itBegin = spline->GetPoint(length);
    auto itEnd = spline->GetPoint(length + maxSegmentLength);
    if (itEnd.BeginAgain())
    {
      double const lastSegmentLength = spline->GetLengths().back();
      itEnd = spline->GetPoint(splineFullLength - lastSegmentLength / 2.0);
      splineRect.Add(spline->GetPath().back());
    }

    spline->ForEachNode(itBegin, itEnd, [&splineRect](m2::PointD const & pt)
    {
      splineRect.Add(pt);
    });

    length += maxSegmentLength;

    if (!func(splineRect))
      return;
  }
}

void CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey,
                    ref_ptr<dp::TextureManager> textures, kml::TrackIdCollection const & linesId,
                    UserLinesRenderCollection & renderParams, dp::Batcher & batcher)
{
  ASSERT_GREATER(tileKey.m_zoomLevel, 0, ());
  ASSERT_LESS_OR_EQUAL(tileKey.m_zoomLevel, scales::GetUpperStyleScale(), ());

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  bool const simplify = tileKey.m_zoomLevel <= kLineSimplifyLevelEnd;

  double sqrScale = 1.0;
  if (simplify)
  {
    double const currentScaleGtoP = 1.0 / GetScreenScale(tileKey.m_zoomLevel);
    sqrScale = currentScaleGtoP * currentScaleGtoP;
  }

  for (auto id : linesId)
  {
    auto const it = renderParams.find(id);
    if (it == renderParams.end())
      continue;

    UserLineRenderParams const & renderInfo = *it->second;

    m2::RectD const tileRect = tileKey.GetGlobalRect();

    double const maxLength = MercatorBounds::kRangeX / (1 << (tileKey.m_zoomLevel - 1));

    bool intersected = false;
    ProcessSplineSegmentRects(renderInfo.m_spline, maxLength,
                              [&tileRect, &intersected](m2::RectD const & segmentRect)
    {
      if (segmentRect.IsIntersect(tileRect))
        intersected = true;
      return !intersected;
    });

    if (!intersected)
      continue;

    m2::SharedSpline spline = renderInfo.m_spline;
    if (simplify)
      spline = SimplifySpline(renderInfo, sqrScale);

    auto const clippedSplines = m2::ClipSplineByRect(tileRect, spline);
    for (auto const & clippedSpline : clippedSplines)
    {
      for (auto const & layer : renderInfo.m_layers)
      {
        LineViewParams params;
        params.m_tileCenter = tileKey.GetGlobalRect().Center();
        params.m_baseGtoPScale = 1.0f;
        params.m_cap = dp::RoundCap;
        params.m_join = dp::RoundJoin;
        params.m_color = layer.m_color;
        params.m_depthTestEnabled = true;
        params.m_depth = layer.m_depth;
        params.m_depthLayer = renderInfo.m_depthLayer;
        params.m_width = static_cast<float>(layer.m_width * vs *
          kLineWidthZoomFactor[tileKey.m_zoomLevel - 1]);
        params.m_minVisibleScale = 1;
        params.m_rank = 0;

        LineShape(clippedSpline, params).Draw(context, make_ref(&batcher), textures);
      }
    }
  }
}
} // namespace df
