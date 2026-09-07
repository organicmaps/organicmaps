#include "transit_scheme_builder.hpp"

#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/text_layout.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/render_bucket.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{
std::array<float, 20> constexpr kTransitLinesWidthInPixel = {
    // 1   2     3     4     5     6     7     8     9    10
    0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f, 1.00f,
    // 11  12    13    14    15    16    17    18    19     20
    1.32f, 1.60f, 2.00f, 2.40f, 2.80f, 3.44f, 4.00f, 4.40f, 4.64f, 4.64f};

namespace
{
float constexpr kBaseLineDepth = 0.0f;
float constexpr kDepthPerLine = 1.0f;
float constexpr kBaseMarkerDepth = 300.0f;
int constexpr kFinalStationMinZoomLevel = kTransitSchemeMinZoomLevel;
int constexpr kTransferMinZoomLevel = kTransitSchemeMinZoomLevel + 1;
uint16_t constexpr kFinalStationPriorityInc = 2;

float constexpr kStopScale = 2.5f;
float constexpr kTransferScale = 3.0f;

float constexpr kOuterMarkerDepth = kBaseMarkerDepth + 0.5f;
float constexpr kInnerMarkerDepth = kBaseMarkerDepth + 1.0f;
uint32_t constexpr kTransitStubOverlayIndex = 1000;
uint32_t constexpr kTransitOverlayIndex = 1001;

std::string const kTransitMarkText = "TransitMarkPrimaryText";
std::string const kTransitMarkTextOutline = "TransitMarkPrimaryTextOutline";
std::string const kTransitTransferOuterColor = "TransitTransferOuterMarker";
std::string const kTransitTransferInnerColor = "TransitTransferInnerMarker";
std::string const kTransitStopInnerColor = "TransitStopInnerMarker";

float constexpr kTransitMarkTextSize = 11.0f;

// Only rail types are drawn on the transit layer; bus/tram/trolleybus lines in the section are skipped.
bool IsSubwayLayerLine(routing::transit::Line const & line)
{
  auto const & type = line.GetType();
  return type == "subway" || type == "train" || type == "light_rail" || type == "monorail";
}

struct TransitStaticVertex
{
  using TPosition = glsl::vec3;
  using TNormal = glsl::vec4;
  using TColor = glsl::vec4;

  TransitStaticVertex(TPosition const & position, TNormal const & normal, TColor const & color)
    : m_position(position)
    , m_normal(normal)
    , m_color(color)
  {}

  TPosition m_position;
  TNormal m_normal;
  TColor m_color;
};

struct SchemeSegment
{
  glsl::vec2 m_p1;
  glsl::vec2 m_p2;
  glsl::vec2 m_tangent;
  glsl::vec2 m_leftNormal;
  glsl::vec2 m_rightNormal;
};

using TGeometryBuffer = std::vector<TransitStaticVertex>;

dp::BindingInfo const & GetTransitStaticBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<TransitStaticVertex> filler(3);
    filler.FillDecl<TransitStaticVertex::TPosition>("a_position");
    filler.FillDecl<TransitStaticVertex::TNormal>("a_normal");
    filler.FillDecl<TransitStaticVertex::TColor>("a_color");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

void GenerateLineCaps(ref_ptr<dp::GraphicsContext> context, std::vector<SchemeSegment> const & segments,
                      glsl::vec4 const & color, float lineOffset, float halfWidth, float depth, dp::Batcher & batcher)
{
  using TV = TransitStaticVertex;

  TGeometryBuffer geometry;
  geometry.reserve(segments.size() * 3);

  for (auto const & segment : segments)
  {
    // Here we use an equilateral triangle to render a circle (incircle of a triangle).
    static float const kSqrt3 = sqrt(3.0f);
    auto const offset = lineOffset * segment.m_rightNormal;
    auto const pivot = glsl::vec3(segment.m_p2, depth);

    auto const n1 = glsl::vec2(-2.0 * halfWidth, -halfWidth);
    auto const n2 = glsl::vec2(2.0 * halfWidth, -halfWidth);
    auto const n3 = glsl::vec2(0.0f, (2.0f * kSqrt3 - 1.0f) * halfWidth);

    geometry.emplace_back(pivot, TV::TNormal(-offset, n1), color);
    geometry.emplace_back(pivot, TV::TNormal(-offset, n2), color);
    geometry.emplace_back(pivot, TV::TNormal(-offset, n3), color);
  }

  dp::AttributeProvider provider(1 /* stream count */, static_cast<uint32_t>(geometry.size()));
  provider.InitStream(0 /* stream index */, GetTransitStaticBindingInfo(), make_ref(geometry.data()));
  auto state = CreateRenderState(gpu::Program::TransitCircle, DepthLayer::TransitSchemeLayer);
  batcher.InsertTriangleList(context, state, make_ref(&provider));
}

struct TitleInfo
{
  explicit TitleInfo(std::string const & text) : m_text(text) {}

  std::string m_text;
  size_t m_rowsCount = 0;
  m2::PointF m_pixelSize;
  m2::PointF m_offset;
  dp::Anchor m_anchor = dp::Left;
};

std::vector<TitleInfo> GetTitles(StopNodeParamsSubway const & stopParams)
{
  std::vector<TitleInfo> titles;

  for (auto const & stopInfo : stopParams.m_stopsInfo)
  {
    if (stopInfo.second.m_name.empty())
      continue;

    bool isUnique = true;
    for (auto const & title : titles)
    {
      if (title.m_text == stopInfo.second.m_name)
      {
        isUnique = false;
        break;
      }
    }
    if (isUnique)
      titles.emplace_back(stopInfo.second.m_name);
  }

  return titles;
}

void PlaceTitles(std::vector<TitleInfo> & titles, float textSize, ref_ptr<dp::TextureManager> textures)
{
  if (titles.size() < 2)
    return;

  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  size_t summaryRowsCount = 0;
  for (auto & name : titles)
  {
    // Transit titles do not carry a lang code; pass kUnsupportedLanguageCode so HarfBuzz
    // applies no `locl` substitutions.
    StraightTextLayout layout(name.m_text, textSize, textures, dp::Left, false /* forceNoWrap */,
                              StringUtf8Multilang::kUnsupportedLanguageCode);
    name.m_pixelSize = layout.GetPixelSize() + m2::PointF(4.0f * vs, 4.0f * vs);
    name.m_rowsCount = layout.GetRowsCount();
    summaryRowsCount += layout.GetRowsCount();
  }

  auto const rightRowsCount = summaryRowsCount > 3 ? (summaryRowsCount + 1) / 2 : summaryRowsCount;
  float rightHeight = 0.0f;
  float leftHeight = 0.0f;
  size_t rowsCount = 0;
  size_t rightTitlesCount = 0;
  for (size_t i = 0; i < titles.size(); ++i)
  {
    if (rowsCount < rightRowsCount)
    {
      rightHeight += titles[i].m_pixelSize.y;
      ++rightTitlesCount;
    }
    else
    {
      leftHeight += titles[i].m_pixelSize.y;
    }
    rowsCount += titles[i].m_rowsCount;
  }

  float currentOffset = -rightHeight / 2.0f;
  for (size_t i = 0; i < rightTitlesCount; ++i)
  {
    titles[i].m_anchor = dp::Left;
    titles[i].m_offset.y = currentOffset + titles[i].m_pixelSize.y / 2.0f;
    currentOffset += titles[i].m_pixelSize.y;
  }

  currentOffset = -leftHeight / 2.0f;
  for (size_t i = rightTitlesCount; i < titles.size(); ++i)
  {
    titles[i].m_anchor = dp::Right;
    titles[i].m_offset.y = currentOffset + titles[i].m_pixelSize.y / 2.0f;
    currentOffset += titles[i].m_pixelSize.y;
  }
}

std::vector<m2::PointF> GetTransitMarkerSizes(float markerScale, float maxRouteWidth)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  std::vector<m2::PointF> markerSizes;
  markerSizes.reserve(df::kTransitLinesWidthInPixel.size());

  for (auto const halfWidth : df::kTransitLinesWidthInPixel)
  {
    float const d = 2.0f * std::min(halfWidth * vs, maxRouteWidth * 0.5f) * markerScale;
    markerSizes.emplace_back(d, d);
  }

  return markerSizes;
}

uint32_t GetRouteId(routing::transit::LineId lineId)
{
  return static_cast<uint32_t>(lineId >> 4);
}

void FillStopParamsSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSet::MwmId const & mwmId,
                          routing::transit::Stop const & stop, StopNodeParamsSubway & stopParams)
{
  FeatureID featureId;
  std::string title;
  if (stop.GetFeatureId() != kInvalidFeatureId)
  {
    featureId = FeatureID(mwmId, stop.GetFeatureId());
    title = transitDisplayInfo.m_features.at(featureId).m_title;
  }
  stopParams.m_isTransfer = false;
  stopParams.m_pivot = stop.GetPoint();
  for (auto lineId : stop.GetLineIds())
  {
    // A stop may be shared with lines which are not rendered on the layer (e.g. buses).
    auto const itLine = transitDisplayInfo.m_linesSubway.find(lineId);
    if (itLine == transitDisplayInfo.m_linesSubway.end() || !IsSubwayLayerLine(itLine->second))
      continue;

    StopInfo & info = stopParams.m_stopsInfo[GetRouteId(lineId)];
    info.m_featureId = featureId;
    info.m_name = title;
    info.m_lines.insert(lineId);
  }
}

bool FindLongerPath(routing::transit::StopId stop1Id, routing::transit::StopId stop2Id,
                    std::vector<routing::transit::StopId> const & sameStops, size_t & stop1Ind, size_t & stop2Ind)
{
  stop1Ind = std::numeric_limits<size_t>::max();
  stop2Ind = std::numeric_limits<size_t>::max();

  for (size_t stopInd = 0; stopInd < sameStops.size(); ++stopInd)
    if (sameStops[stopInd] == stop1Id)
      stop1Ind = stopInd;
    else if (sameStops[stopInd] == stop2Id)
      stop2Ind = stopInd;

  if (stop1Ind < sameStops.size() || stop2Ind < sameStops.size())
  {
    if (stop1Ind > stop2Ind)
      std::swap(stop1Ind, stop2Ind);

    if (stop1Ind < sameStops.size() && stop2Ind < sameStops.size() && stop2Ind - stop1Ind > 1)
      return true;
  }
  return false;
}

int GetMinVisibleScale(bool isMain, int mainScale)
{
  // Show regular stops later (+1) than main (terminal, transfer) stops.
  return isMain ? mainScale : mainScale + 1;
}
}  // namespace

void TransitSchemeBuilder::UpdateSchemes(ref_ptr<dp::GraphicsContext> context,
                                         TransitDisplayInfos const & transitDisplayInfos,
                                         ref_ptr<dp::TextureManager> textures)
{
  for (auto const & [mwmId, transitDisplayInfoPtr] : transitDisplayInfos)
  {
    if (!transitDisplayInfoPtr)
      continue;

    auto const & transitDisplayInfo = *transitDisplayInfoPtr.get();

    MwmSchemeData & scheme = m_schemes[mwmId];
    CollectStopsSubway(transitDisplayInfo, mwmId, scheme);
    CollectLinesSubway(transitDisplayInfo, scheme);
    CollectShapesSubway(transitDisplayInfo, scheme);

    PrepareSchemeSubway(scheme);
    BuildScheme(context, mwmId, textures);
  }
}

void TransitSchemeBuilder::Clear()
{
  m_schemes.clear();
}

void TransitSchemeBuilder::Clear(MwmSet::MwmId const & mwmId)
{
  m_schemes.erase(mwmId);
}

void TransitSchemeBuilder::RebuildSchemes(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures)
{
  for (auto const & mwmScheme : m_schemes)
    BuildScheme(context, mwmScheme.first, textures);
}

void TransitSchemeBuilder::BuildScheme(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                                       ref_ptr<dp::TextureManager> textures)
{
  if (m_schemes.find(mwmId) == m_schemes.end())
    return;

  ++m_recacheId;
  GenerateLines(context, mwmId);
  GenerateStops(context, mwmId, textures);
}

void TransitSchemeBuilder::CollectStopsSubway(TransitDisplayInfo const & transitDisplayInfo,
                                              MwmSet::MwmId const & mwmId, MwmSchemeData & scheme)
{
  // Lines not present on the layer (e.g. buses and trams) have no stop ranges in
  // |m_linesSubway| of the scheme, so their exclusive stops are skipped here too.
  auto const hasLayerLine = [&transitDisplayInfo](routing::transit::Stop const & stop)
  {
    for (auto lineId : stop.GetLineIds())
    {
      auto const it = transitDisplayInfo.m_linesSubway.find(lineId);
      if (it != transitDisplayInfo.m_linesSubway.end() && IsSubwayLayerLine(it->second))
        return true;
    }
    return false;
  };

  for (auto const & stopInfo : transitDisplayInfo.m_stopsSubway)
  {
    routing::transit::Stop const & stop = stopInfo.second;
    if (stop.GetTransferId() != routing::transit::kInvalidTransferId)
      continue;
    if (!hasLayerLine(stop))
      continue;
    auto & stopNode = scheme.m_stopsSubway[stop.GetId()];
    FillStopParamsSubway(transitDisplayInfo, mwmId, stop, stopNode);
  }

  for (auto const & stopInfo : transitDisplayInfo.m_transfersSubway)
  {
    routing::transit::Transfer const & transfer = stopInfo.second;
    auto & stopNode = scheme.m_transfersSubway[transfer.GetId()];

    for (auto stopId : transfer.GetStopIds())
    {
      if (transitDisplayInfo.m_stopsSubway.find(stopId) == transitDisplayInfo.m_stopsSubway.end())
      {
        LOG(LWARNING, ("Invalid stop", stopId, "in transfer", transfer.GetId()));
        continue;
      }
      routing::transit::Stop const & stop = transitDisplayInfo.m_stopsSubway.at(stopId);
      FillStopParamsSubway(transitDisplayInfo, mwmId, stop, stopNode);
    }
    stopNode.m_isTransfer = true;
    stopNode.m_pivot = transfer.GetPoint();
  }
}

void TransitSchemeBuilder::CollectLinesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme)
{
  std::multimap<size_t, routing::transit::LineId> linesLengths;
  for (auto const & line : transitDisplayInfo.m_linesSubway)
  {
    // Bus and tram lines are not rendered on the subway layer.
    if (!IsSubwayLayerLine(line.second))
      continue;

    auto const lineId = line.second.GetId();
    size_t stopsCount = 0;
    auto const & stopsRanges = line.second.GetStopIds();
    for (auto const & stops : stopsRanges)
      stopsCount += stops.size();
    linesLengths.insert(std::make_pair(stopsCount, lineId));
  }
  float depth = kBaseLineDepth;
  for (auto const & pair : linesLengths)
  {
    auto const lineId = pair.second;

    scheme.m_linesSubway[lineId] = LineParams(transitDisplayInfo.m_linesSubway.at(lineId).GetColor(), depth);

    depth += kDepthPerLine;
  }
}

void TransitSchemeBuilder::CollectShapesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme)
{
  std::map<uint32_t, std::vector<routing::transit::LineId>> roads;

  for (auto const & line : transitDisplayInfo.m_linesSubway)
  {
    if (!IsSubwayLayerLine(line.second))
      continue;

    auto const lineId = line.second.GetId();
    auto const roadId = GetRouteId(lineId);
    roads[roadId].push_back(lineId);
  }

  for (auto const & line : transitDisplayInfo.m_linesSubway)
  {
    if (!IsSubwayLayerLine(line.second))
      continue;

    auto const lineId = line.second.GetId();
    auto const roadId = GetRouteId(lineId);

    auto const & stopsRanges = line.second.GetStopIds();
    for (auto const & stops : stopsRanges)
      for (size_t i = 1; i < stops.size(); ++i)
        FindShapes(stops[i - 1], stops[i], lineId, roads[roadId], transitDisplayInfo, scheme);
  }
}

void TransitSchemeBuilder::FindShapes(routing::transit::StopId stop1Id, routing::transit::StopId stop2Id,
                                      routing::transit::LineId lineId,
                                      std::vector<routing::transit::LineId> const & sameLines,
                                      TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme)
{
  bool shapeAdded = false;

  for (auto const & sameLineId : sameLines)
  {
    if (sameLineId == lineId)
      continue;

    auto const & sameLine = transitDisplayInfo.m_linesSubway.at(sameLineId);
    auto const & sameStopsRanges = sameLine.GetStopIds();
    for (auto const & sameStops : sameStopsRanges)
    {
      size_t stop1Ind;
      size_t stop2Ind;
      if (FindLongerPath(stop1Id, stop2Id, sameStops, stop1Ind, stop2Ind))
      {
        shapeAdded = true;
        for (size_t stopInd = stop1Ind; stopInd < stop2Ind; ++stopInd)
          AddShape(transitDisplayInfo, sameStops[stopInd], sameStops[stopInd + 1], lineId, scheme);
      }
      if (stop1Ind < sameStops.size() || stop2Ind < sameStops.size())
        break;
    }

    if (shapeAdded)
      break;
  }

  if (!shapeAdded)
    AddShape(transitDisplayInfo, stop1Id, stop2Id, lineId, scheme);
}

void TransitSchemeBuilder::AddShape(TransitDisplayInfo const & transitDisplayInfo, routing::transit::StopId stop1Id,
                                    routing::transit::StopId stop2Id, routing::transit::LineId lineId,
                                    MwmSchemeData & scheme)
{
  auto const stop1It = transitDisplayInfo.m_stopsSubway.find(stop1Id);
  ASSERT(stop1It != transitDisplayInfo.m_stopsSubway.end(), (stop1Id));

  auto const stop2It = transitDisplayInfo.m_stopsSubway.find(stop2Id);
  ASSERT(stop2It != transitDisplayInfo.m_stopsSubway.end(), (stop2Id));

  auto const transfer1Id = stop1It->second.GetTransferId();
  auto const transfer2Id = stop2It->second.GetTransferId();

  auto shapeId = routing::transit::ShapeId(transfer1Id != routing::transit::kInvalidTransferId ? transfer1Id : stop1Id,
                                           transfer2Id != routing::transit::kInvalidTransferId ? transfer2Id : stop2Id);
  auto it = transitDisplayInfo.m_shapesSubway.find(shapeId);
  bool isForward = true;
  if (it == transitDisplayInfo.m_shapesSubway.end())
  {
    isForward = false;
    shapeId = routing::transit::ShapeId(shapeId.GetStop2Id(), shapeId.GetStop1Id());
    it = transitDisplayInfo.m_shapesSubway.find(shapeId);
  }

  if (it == transitDisplayInfo.m_shapesSubway.end())
    return;

  auto const itScheme = scheme.m_shapesSubway.find(shapeId);
  if (itScheme == scheme.m_shapesSubway.end())
  {
    auto const & polyline = transitDisplayInfo.m_shapesSubway.at(it->first).GetPolyline();
    if (isForward)
      scheme.m_shapesSubway[shapeId].m_forwardLines.push_back(lineId);
    else
      scheme.m_shapesSubway[shapeId].m_backwardLines.push_back(lineId);
    scheme.m_shapesSubway[shapeId].m_polyline = polyline;
  }
  else
  {
    for (auto id : itScheme->second.m_forwardLines)
      if (GetRouteId(id) == GetRouteId(lineId))
        return;
    for (auto id : itScheme->second.m_backwardLines)
      if (GetRouteId(id) == GetRouteId(lineId))
        return;

    if (isForward)
      itScheme->second.m_forwardLines.push_back(lineId);
    else
      itScheme->second.m_backwardLines.push_back(lineId);
  }
}

void TransitSchemeBuilder::PrepareSchemeSubway(MwmSchemeData & scheme)
{
  m2::RectD boundingRect;

  for (auto const & shape : scheme.m_shapesSubway)
  {
    auto const stop1 = shape.first.GetStop1Id();
    auto const stop2 = shape.first.GetStop2Id();
    StopNodeParamsSubway & params1 = (scheme.m_stopsSubway.find(stop1) == scheme.m_stopsSubway.end())
                                       ? scheme.m_transfersSubway[stop1]
                                       : scheme.m_stopsSubway[stop1];
    StopNodeParamsSubway & params2 = (scheme.m_stopsSubway.find(stop2) == scheme.m_stopsSubway.end())
                                       ? scheme.m_transfersSubway[stop2]
                                       : scheme.m_stopsSubway[stop2];

    auto const linesCount = shape.second.m_forwardLines.size() + shape.second.m_backwardLines.size();

    auto const sz = shape.second.m_polyline.size();

    auto const dir1 = (shape.second.m_polyline[1] - shape.second.m_polyline[0]).Normalize();
    auto const dir2 = (shape.second.m_polyline[sz - 2] - shape.second.m_polyline[sz - 1]).Normalize();

    params1.m_shapesInfo[shape.first].m_linesCount = linesCount;
    params1.m_shapesInfo[shape.first].m_direction = dir1;

    params2.m_shapesInfo[shape.first].m_linesCount = linesCount;
    params2.m_shapesInfo[shape.first].m_direction = dir2;

    for (auto const & pt : shape.second.m_polyline)
      boundingRect.Add(pt);
  }

  scheme.m_pivot = boundingRect.Center();
}

void TransitSchemeBuilder::GenerateLines(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId)
{
  MwmSchemeData const & scheme = m_schemes[mwmId];

  uint32_t constexpr kBatchSize = 65000;
  dp::Batcher batcher(kBatchSize, kBatchSize);
  batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Transit));
  {
    dp::SessionGuard guard(context, batcher,
                           [this, &mwmId, &scheme](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      TransitRenderData::Type type = TransitRenderData::Type::Lines;
      if (state.GetProgram<gpu::Program>() == gpu::Program::TransitCircle)
        type = TransitRenderData::Type::LinesCaps;
      TransitRenderData renderData(type, state, m_recacheId, mwmId, scheme.m_pivot, std::move(b));
      m_flushRenderDataFn(std::move(renderData));
    });

    for (auto const & shape : scheme.m_shapesSubway)
    {
      size_t const linesCount = shape.second.m_forwardLines.size() + shape.second.m_backwardLines.size();
      float shapeOffset = -static_cast<float>(linesCount / 2) * 2.0f - static_cast<float>(linesCount % 2) + 1.0f;
      float constexpr shapeOffsetIncrement = 2.0f;

      auto const drawLine = [&](routing::transit::LineId lineId)
      {
        auto const & line = scheme.m_linesSubway.at(lineId);
        GenerateLine(context, shape.second.m_polyline, scheme.m_pivot,
                     GetColorConstant(df::GetTransitColorName(line.m_color)), shapeOffset, kTransitLineHalfWidth,
                     line.m_depth, batcher);
        shapeOffset += shapeOffsetIncrement;
      };

      for (auto const lineId : shape.second.m_forwardLines)
        drawLine(lineId);

      for (auto it = shape.second.m_backwardLines.rbegin(); it != shape.second.m_backwardLines.rend(); ++it)
        drawLine(*it);
    }
  }
}

void TransitSchemeBuilder::GenerateStops(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                                         ref_ptr<dp::TextureManager> textures)
{
  MwmSchemeData const & scheme = m_schemes[mwmId];

  auto const flusher = [this, &mwmId, &scheme](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
  {
    TransitRenderData::Type type = TransitRenderData::Type::Stubs;
    if (state.GetProgram<gpu::Program>() == gpu::Program::TransitMarker)
      type = TransitRenderData::Type::Markers;
    else if (state.GetProgram<gpu::Program>() == gpu::Program::TextOutlined)
      type = TransitRenderData::Type::Text;

    TransitRenderData renderData(type, state, m_recacheId, mwmId, scheme.m_pivot, std::move(b));
    m_flushRenderDataFn(std::move(renderData));
  };

  uint32_t constexpr kBatchSize = 5000;
  dp::Batcher batcher(kBatchSize, kBatchSize);

  batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Transit));
  dp::SessionGuard guard(context, batcher, flusher);

  std::vector<m2::PointF> const transferMarkerSizes = GetTransitMarkerSizes(kTransferScale, 1000);
  std::vector<m2::PointF> const stopMarkerSizes = GetTransitMarkerSizes(kStopScale, 1000);

  for (auto const & stop : scheme.m_stopsSubway)
  {
    GenerateStop(context, stop.second, scheme.m_pivot, scheme.m_linesSubway, batcher);
    GenerateTitles(context, stop.second, scheme.m_pivot, stopMarkerSizes, textures, batcher);
  }

  for (auto const & transfer : scheme.m_transfersSubway)
  {
    GenerateTransfer(context, transfer.second, scheme.m_pivot, batcher);
    GenerateTitles(context, transfer.second, scheme.m_pivot, transferMarkerSizes, textures, batcher);
  }
}

void TransitSchemeBuilder::BuildFromRouteTransit(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                                                 TransitInfo const & info, ref_ptr<dp::TextureManager> textures)
{
  if (info.IsEmpty())
    return;

  ++m_recacheId;

  // Pivot: centroid of the combined bbox of all route points and stops.
  m2::RectD bbox;
  for (auto const & route : info.m_routes)
    for (auto const & p : route.m_polyline)
      bbox.Add(p);
  for (auto const & stop : info.m_stops)
    bbox.Add(stop.m_pos);
  if (!bbox.IsValid())
    return;

  m2::PointD const pivot = bbox.Center();

  uint32_t constexpr kBatchSize = 65000;
  dp::Batcher batcher(kBatchSize, kBatchSize);
  batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Transit));
  {
    dp::SessionGuard guard(context, batcher,
                           [this, &mwmId, &pivot](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      TransitRenderData::Type type = TransitRenderData::Type::Lines;
      auto const prog = state.GetProgram<gpu::Program>();
      if (prog == gpu::Program::TransitCircle)
        type = TransitRenderData::Type::LinesCaps;
      else if (prog == gpu::Program::TransitMarker)
        type = TransitRenderData::Type::Markers;
      else if (prog == gpu::Program::TextOutlined)
        type = TransitRenderData::Type::Text;

      TransitRenderData renderData(type, state, m_recacheId, mwmId, pivot, std::move(b));
      m_flushRenderDataFn(std::move(renderData));
    });

    float constexpr kDepth = 0.0f;
    for (auto const & route : info.m_routes)
    {
      if (route.m_polyline.size() < 2)
        continue;
      GenerateLine(context, route.m_polyline, pivot, info.m_color, 0.0f /* lineOffset */, kTransitLineHalfWidth, kDepth,
                   batcher);
    }

    dp::Color const highlightColor = info.GetHighlightColor();
    for (auto const & stop : info.m_stops)
    {
      dp::Color const markerColor = stop.m_highlight ? highlightColor : info.m_color;
      GenerateMarker(context, MapShape::ConvertToLocal(stop.m_pos, pivot, kShapeCoordScalar),
                     m2::PointD(1.0, 0.0) /* widthDir */, 1.0f /* linesCountWidth */, 1.0f /* linesCountHeight */,
                     kStopScale, kStopScale, kDepth, markerColor, batcher);
    }

    // Stop name labels — emitted into the same batcher; the session guard flusher routes them
    // to TransitRenderData::Type::Text via the TextOutlined program.
    auto const vs = static_cast<float>(VisualParams::Instance().GetVisualScale());
    std::vector<m2::PointF> const stopMarkerSizes = GetTransitMarkerSizes(kStopScale, 1000);

    dp::FontDecl const regularFont(GetColorConstant(kTransitMarkText), kTransitMarkTextSize * vs,
                                   GetColorConstant(kTransitMarkTextOutline));
    dp::FontDecl const highlightedFont(highlightColor, kTransitMarkTextSize * vs,
                                       GetColorConstant(kTransitMarkTextOutline));

    // Highlighted stops share the Transfer priority range so they win overlay-tree displacement
    // against regular stops (which use the Stop range). Two independent counters per kind.
    uint16_t const regularPrio = std::to_underlying(Priority::StopMin);
    uint16_t const highlightedPrio = std::to_underlying(Priority::TransferMin);

    TextViewParams textParams;
    textParams.m_tileCenter = pivot;
    textParams.m_titleDecl.m_primaryOptional = true;
    textParams.m_titleDecl.m_anchor = dp::Left;
    textParams.m_depthTestEnabled = false;
    textParams.m_depthLayer = DepthLayer::TransitSchemeLayer;
    textParams.m_specialDisplacement = SpecialDisplacement::SpecialModeUserMark;
    textParams.m_startOverlayRank = dp::OverlayRank0;

    for (auto const & stop : info.m_stops)
    {
      if (stop.m_name.empty())
        continue;

      textParams.m_featureId = stop.m_featureId;
      textParams.m_titleDecl.m_primaryTextFont = stop.m_highlight ? highlightedFont : regularFont;
      textParams.m_titleDecl.m_primaryText = stop.m_name;
      textParams.m_specialPriority = stop.m_highlight ? highlightedPrio : regularPrio;
      textParams.m_minVisibleScale = GetMinVisibleScale(stop.m_highlight, info.m_minZoomLevel);

      // TextShape takes the GLOBAL mercator point, not a local (pivot-relative) one.
      TextShape(stop.m_pos, textParams, TileKey(), stopMarkerSizes, m2::PointF(0.0f, 0.0f), dp::Center,
                kTransitOverlayIndex)
          .Draw(context, &batcher, textures);
    }
  }
}

void TransitSchemeBuilder::GenerateMarker(ref_ptr<dp::GraphicsContext> context, m2::PointD const & pt,
                                          m2::PointD widthDir, float linesCountWidth, float linesCountHeight,
                                          float scaleWidth, float scaleHeight, float depth, dp::Color const & color,
                                          dp::Batcher & batcher)
{
  using TV = TransitStaticVertex;

  scaleWidth = (scaleWidth - 1.0f) / linesCountWidth + 1.0f;
  scaleHeight = (scaleHeight - 1.0f) / linesCountHeight + 1.0f;

  widthDir.y = -widthDir.y;
  m2::PointD heightDir = widthDir.Ort();

  auto const v1 = -widthDir * scaleWidth * linesCountWidth - heightDir * scaleHeight * linesCountHeight;
  auto const v2 = widthDir * scaleWidth * linesCountWidth - heightDir * scaleHeight * linesCountHeight;
  auto const v3 = widthDir * scaleWidth * linesCountWidth + heightDir * scaleHeight * linesCountHeight;
  auto const v4 = -widthDir * scaleWidth * linesCountWidth + heightDir * scaleHeight * linesCountHeight;

  glsl::vec3 const pos(pt.x, pt.y, depth);
  auto const colorVal = glsl::vec4(color.GetRedF(), color.GetGreenF(), color.GetBlueF(), 1.0f /* alpha */);

  TGeometryBuffer geometry;
  geometry.reserve(6);
  geometry.emplace_back(pos, TV::TNormal(v1.x, v1.y, -linesCountWidth, -linesCountHeight), colorVal);
  geometry.emplace_back(pos, TV::TNormal(v2.x, v2.y, linesCountWidth, -linesCountHeight), colorVal);
  geometry.emplace_back(pos, TV::TNormal(v3.x, v3.y, linesCountWidth, linesCountHeight), colorVal);
  geometry.emplace_back(pos, TV::TNormal(v1.x, v1.y, -linesCountWidth, -linesCountHeight), colorVal);
  geometry.emplace_back(pos, TV::TNormal(v3.x, v3.y, linesCountWidth, linesCountHeight), colorVal);
  geometry.emplace_back(pos, TV::TNormal(v4.x, v4.y, -linesCountWidth, linesCountHeight), colorVal);

  dp::AttributeProvider provider(1 /* stream count */, static_cast<uint32_t>(geometry.size()));
  provider.InitStream(0 /* stream index */, GetTransitStaticBindingInfo(), make_ref(geometry.data()));
  auto state = CreateRenderState(gpu::Program::TransitMarker, DepthLayer::TransitSchemeLayer);
  batcher.InsertTriangleList(context, state, make_ref(&provider));
}

void TransitSchemeBuilder::GenerateLine(ref_ptr<dp::GraphicsContext> context, std::vector<m2::PointD> const & path,
                                        m2::PointD const & pivot, dp::Color const & colorConst, float lineOffset,
                                        float halfWidth, float depth, dp::Batcher & batcher)
{
  using TV = TransitStaticVertex;

  TGeometryBuffer geometry;
  auto const color = glsl::vec4(colorConst.GetRedF(), colorConst.GetGreenF(), colorConst.GetBlueF(), 1.0f /* alpha */);
  size_t const kAverageSize = path.size() * 6;
  size_t const kAverageCapSize = 12;
  geometry.reserve(kAverageSize + kAverageCapSize * 2);

  std::vector<SchemeSegment> segments;
  segments.reserve(path.size() - 1);

  for (size_t i = 1; i < path.size(); ++i)
  {
    if (path[i].EqualDxDy(path[i - 1], 1.0e-5))
      continue;

    SchemeSegment segment;
    segment.m_p1 = glsl::ToVec2(MapShape::ConvertToLocal(path[i - 1], pivot, kShapeCoordScalar));
    segment.m_p2 = glsl::ToVec2(MapShape::ConvertToLocal(path[i], pivot, kShapeCoordScalar));
    CalculateTangentAndNormals(segment.m_p1, segment.m_p2, segment.m_tangent, segment.m_leftNormal,
                               segment.m_rightNormal);

    auto const startPivot = glsl::vec3(segment.m_p1, depth);
    auto const endPivot = glsl::vec3(segment.m_p2, depth);
    auto const offset = lineOffset * segment.m_rightNormal;

    geometry.emplace_back(startPivot, TV::TNormal(segment.m_rightNormal * halfWidth - offset, -halfWidth, 0.0), color);
    geometry.emplace_back(startPivot, TV::TNormal(segment.m_leftNormal * halfWidth - offset, halfWidth, 0.0), color);
    geometry.emplace_back(endPivot, TV::TNormal(segment.m_rightNormal * halfWidth - offset, -halfWidth, 0.0), color);
    geometry.emplace_back(endPivot, TV::TNormal(segment.m_rightNormal * halfWidth - offset, -halfWidth, 0.0), color);
    geometry.emplace_back(startPivot, TV::TNormal(segment.m_leftNormal * halfWidth - offset, halfWidth, 0.0), color);
    geometry.emplace_back(endPivot, TV::TNormal(segment.m_leftNormal * halfWidth - offset, halfWidth, 0.0), color);

    segments.emplace_back(std::move(segment));
  }

  dp::AttributeProvider provider(1 /* stream count */, static_cast<uint32_t>(geometry.size()));
  provider.InitStream(0 /* stream index */, GetTransitStaticBindingInfo(), make_ref(geometry.data()));
  auto state = CreateRenderState(gpu::Program::Transit, DepthLayer::TransitSchemeLayer);
  batcher.InsertTriangleList(context, state, make_ref(&provider));

  GenerateLineCaps(context, segments, color, lineOffset, halfWidth, depth, batcher);
}

void TransitSchemeBuilder::GenerateStop(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                                        m2::PointD const & pivot,
                                        std::map<routing::transit::LineId, LineParams> const & lines,
                                        dp::Batcher & batcher)
{
  bool const severalRoads = stopParams.m_stopsInfo.size() > 1;

  if (severalRoads)
  {
    GenerateTransfer(context, stopParams, pivot, batcher);
    return;
  }

  float const kInnerScale = 0.8f;
  float const kOuterScale = 2.0f;

  auto const lineId = *stopParams.m_stopsInfo.begin()->second.m_lines.begin();
  auto const colorName = df::GetTransitColorName(lines.at(lineId).m_color);
  auto const outerColor = GetColorConstant(colorName);

  auto const innerColor = GetColorConstant(kTransitStopInnerColor);

  m2::PointD const pt = MapShape::ConvertToLocal(stopParams.m_pivot, pivot, kShapeCoordScalar);

  m2::PointD dir = stopParams.m_shapesInfo.begin()->second.m_direction;

  GenerateMarker(context, pt, dir, 1.0f, 1.0f, kOuterScale, kOuterScale, kOuterMarkerDepth, outerColor, batcher);

  GenerateMarker(context, pt, dir, 1.0f, 1.0f, kInnerScale, kInnerScale, kInnerMarkerDepth, innerColor, batcher);
}

void TransitSchemeBuilder::GenerateTitles(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                                          m2::PointD const & pivot, std::vector<m2::PointF> const & markerSizes,
                                          ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  std::vector<TitleInfo> titles = GetTitles(stopParams);
  if (titles.empty())
    return;

  PlaceTitles(titles, kTransitMarkTextSize * vs, textures);

  auto const featureId = stopParams.m_stopsInfo.begin()->second.m_featureId;

  auto priority = static_cast<uint16_t>(stopParams.m_isTransfer ? Priority::TransferMin : Priority::StopMin);
  priority += static_cast<uint16_t>(stopParams.m_stopsInfo.size());

  auto minVisibleScale = GetMinVisibleScale(stopParams.m_isTransfer, kTransferMinZoomLevel);
  if (stopParams.m_shapesInfo.size() == 1)
  {
    minVisibleScale = std::min(minVisibleScale, kFinalStationMinZoomLevel);
    priority += kFinalStationPriorityInc;
  }

  ASSERT_LESS_OR_EQUAL(priority,
                       static_cast<uint16_t>(stopParams.m_isTransfer ? Priority::TransferMax : Priority::StopMax), ());

  std::vector<m2::PointF> symbolSizes;
  symbolSizes.reserve(markerSizes.size());
  for (auto const & sz : markerSizes)
    symbolSizes.push_back(sz * 1.1f);

  dp::TitleDecl titleDecl;
  titleDecl.m_primaryOptional = true;
  titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(kTransitMarkText);
  titleDecl.m_primaryTextFont.m_outlineColor = df::GetColorConstant(kTransitMarkTextOutline);
  titleDecl.m_primaryTextFont.m_size = kTransitMarkTextSize * vs;
  titleDecl.m_anchor = dp::Left;

  TextViewParams textParams;
  textParams.m_featureId = featureId;
  textParams.m_tileCenter = pivot;
  textParams.m_titleDecl = titleDecl;
  textParams.m_depthTestEnabled = false;
  textParams.m_depthLayer = DepthLayer::TransitSchemeLayer;
  textParams.m_specialDisplacement = SpecialDisplacement::SpecialModeUserMark;
  textParams.m_specialPriority = priority;
  textParams.m_startOverlayRank = dp::OverlayRank0;
  textParams.m_minVisibleScale = minVisibleScale;

  for (auto const & title : titles)
  {
    textParams.m_titleDecl.m_primaryText = title.m_text;
    textParams.m_titleDecl.m_anchor = title.m_anchor;

    TextShape(stopParams.m_pivot, textParams, TileKey(), symbolSizes, title.m_offset, dp::Center, kTransitOverlayIndex)
        .Draw(context, &batcher, textures);
  }

  df::ColoredSymbolViewParams colorParams;
  colorParams.m_radiusInPixels = markerSizes.front().x * 0.5f;
  colorParams.m_color = dp::Color::Transparent();
  colorParams.m_featureId = featureId;
  colorParams.m_tileCenter = pivot;
  colorParams.m_depthTestEnabled = false;
  colorParams.m_depthLayer = DepthLayer::TransitSchemeLayer;
  colorParams.m_specialDisplacement = SpecialDisplacement::SpecialModeUserMark;
  colorParams.m_specialPriority = static_cast<uint16_t>(Priority::Stub);
  colorParams.m_startOverlayRank = dp::OverlayRank0;

  ColoredSymbolShape(stopParams.m_pivot, colorParams, TileKey(), kTransitStubOverlayIndex, markerSizes)
      .Draw(context, &batcher, textures);
}

void TransitSchemeBuilder::GenerateTransfer(ref_ptr<dp::GraphicsContext> context,
                                            StopNodeParamsSubway const & stopParams, m2::PointD const & pivot,
                                            dp::Batcher & batcher)
{
  m2::PointD const pt = MapShape::ConvertToLocal(stopParams.m_pivot, pivot, kShapeCoordScalar);

  size_t maxLinesCount = 0;
  m2::PointD dir;

  for (auto const & shapeInfo : stopParams.m_shapesInfo)
  {
    if (shapeInfo.second.m_linesCount > maxLinesCount)
    {
      dir = shapeInfo.second.m_direction;
      maxLinesCount = shapeInfo.second.m_linesCount;
    }
  }

  // A transfer whose stops belong only to non-layer lines (e.g. bus/tram) has no layer shapes,
  // so |maxLinesCount| stays 0. Skip it to avoid division by zero (NaN geometry) in GenerateMarker().
  if (maxLinesCount == 0)
    return;

  float const kInnerScale = 1.0f;
  float const kOuterScale = 1.5f;

  auto const outerColor = GetColorConstant(kTransitTransferOuterColor);
  auto const innerColor = GetColorConstant(kTransitTransferInnerColor);

  float const widthLinesCount = maxLinesCount > 3 ? 1.6f : 1.0f;
  float const innerScale = maxLinesCount == 1 ? 1.4f : kInnerScale;
  float const outerScale = maxLinesCount == 1 ? 1.9f : kOuterScale;

  GenerateMarker(context, pt, dir, widthLinesCount, maxLinesCount, outerScale, outerScale, kOuterMarkerDepth,
                 outerColor, batcher);

  GenerateMarker(context, pt, dir, widthLinesCount, maxLinesCount, innerScale, innerScale, kInnerMarkerDepth,
                 innerColor, batcher);
}

}  // namespace df
