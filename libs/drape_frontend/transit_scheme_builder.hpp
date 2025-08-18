#pragma once

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/texture_manager.hpp"

#include "transit/transit_display_info.hpp"
#include "transit/transit_version.hpp"

#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace df
{
extern int const kTransitSchemeMinZoomLevel;
extern float const kTransitLineHalfWidth;
extern std::array<float, 20> const kTransitLinesWidthInPixel;

struct TransitRenderData
{
  enum class Type
  {
    LinesCaps,
    Lines,
    Markers,
    Text,
    Stubs
  };

  Type m_type;
  dp::RenderState m_state;
  uint32_t m_recacheId;
  MwmSet::MwmId m_mwmId;
  m2::PointD m_pivot;
  drape_ptr<dp::RenderBucket> m_bucket;

  TransitRenderData(Type type, dp::RenderState const & state, uint32_t recacheId, MwmSet::MwmId const & mwmId,
                    m2::PointD const pivot, drape_ptr<dp::RenderBucket> && bucket)
    : m_type(type)
    , m_state(state)
    , m_recacheId(recacheId)
    , m_mwmId(mwmId)
    , m_pivot(pivot)
    , m_bucket(std::move(bucket))
  {}
};

struct LineParams
{
  LineParams() = default;
  LineParams(std::string const & color, float depth) : m_color(color), m_depth(depth) {}
  std::string m_color;
  float m_depth = 0.0;
  std::vector<::transit::TransitId> m_stopIds;
};

struct ShapeParams
{
  std::vector<routing::transit::LineId> m_forwardLines;
  std::vector<routing::transit::LineId> m_backwardLines;
  std::vector<m2::PointD> m_polyline;
};

struct ShapeInfoSubway
{
  m2::PointD m_direction;
  size_t m_linesCount;
};

struct ShapeInfoPT
{
  ShapeInfoPT() = default;

  ShapeInfoPT(m2::PointD const & dir, std::set<std::string> const & colors) : m_direction(dir), m_colors(colors) {}

  m2::PointD m_direction;
  std::set<std::string> m_colors;
};

struct StopInfo
{
  StopInfo() = default;
  StopInfo(std::string const & name, FeatureID const & featureId) : m_name(name), m_featureId(featureId) {}

  std::string m_name;
  FeatureID m_featureId;
  std::set<routing::transit::LineId> m_lines;
};

struct StopNodeParamsSubway
{
  bool m_isTransfer = false;
  m2::PointD m_pivot;
  std::map<routing::transit::ShapeId, ShapeInfoSubway> m_shapesInfo;
  std::map<uint32_t, StopInfo> m_stopsInfo;
};

struct StopNodeParamsPT
{
  bool m_isTransfer = false;
  bool m_isTerminalStop = false;
  m2::PointD m_pivot;
  // Two types of line shapes going through the stop: inbound and outbound.
  std::vector<ShapeInfoPT> m_shapeInfoIn;
  std::vector<ShapeInfoPT> m_shapeInfoOut;
  // Route id to StopInfo mapping.
  std::map<::transit::TransitId, StopInfo> m_stopsInfo;
};

using IdToIdSet = std::unordered_map<::transit::TransitId, ::transit::IdSet>;

struct RouteSegment
{
  int m_order = 0;
  std::vector<m2::PointD> m_polyline;
};

struct RouteData
{
  std::string m_color;
  float m_depth = 0;
  std::vector<RouteSegment> m_routeShapes;
};

struct LinesDataPT
{
  IdToIdSet m_stopToLineIds;
  std::set<::transit::TransitId> m_terminalStops;
};

class TransitSchemeBuilder
{
public:
  enum class Priority : uint16_t
  {
    Default = 0,
    Stub = 1,
    StopMin = 2,
    StopMax = 30,
    TransferMin = 31,
    TransferMax = 60
  };

  using TFlushRenderDataFn = std::function<void(TransitRenderData && renderData)>;

  explicit TransitSchemeBuilder(TFlushRenderDataFn const & flushFn) : m_flushRenderDataFn(flushFn) {}

  void UpdateSchemes(ref_ptr<dp::GraphicsContext> context, TransitDisplayInfos const & transitDisplayInfos,
                     ref_ptr<dp::TextureManager> textures);

  void RebuildSchemes(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures);

  void Clear();
  void Clear(MwmSet::MwmId const & mwmId);

private:
  struct MwmSchemeData
  {
    m2::PointD m_pivot;

    ::transit::TransitVersion m_transitVersion;

    std::map<routing::transit::LineId, LineParams> m_linesSubway;
    std::map<routing::transit::ShapeId, ShapeParams> m_shapesSubway;
    std::map<routing::transit::StopId, StopNodeParamsSubway> m_stopsSubway;
    std::map<routing::transit::TransferId, StopNodeParamsSubway> m_transfersSubway;

    std::map<::transit::TransitId, LineParams> m_linesPT;
    std::vector<RouteData> m_routeSegmentsPT;
    std::map<::transit::TransitId, StopNodeParamsPT> m_stopsPT;
    std::map<::transit::TransitId, StopNodeParamsPT> m_transfersPT;
  };

  void BuildScheme(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                   ref_ptr<dp::TextureManager> textures);

  void GenerateLinesSubway(MwmSchemeData const & scheme, dp::Batcher & batcher, ref_ptr<dp::GraphicsContext> context);

  void GenerateLinesPT(MwmSchemeData const & scheme, dp::Batcher & batcher, ref_ptr<dp::GraphicsContext> context);

  template <class F, class S, class T, class L>
  void GenerateLocationsWithTitles(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textures,
                                   dp::Batcher & batcher, F && flusher, MwmSchemeData const & scheme, S const & stops,
                                   T const & transfers, L const & lines);

  void CollectStopsSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSet::MwmId const & mwmId,
                          MwmSchemeData & scheme);
  void CollectStopsPT(TransitDisplayInfo const & transitDisplayInfo, LinesDataPT const & linesData,
                      MwmSet::MwmId const & mwmId, MwmSchemeData & scheme);

  void CollectLinesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);
  LinesDataPT CollectLinesPT(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);

  void CollectShapesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);
  void CollectShapesPT(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);

  void FindShapes(routing::transit::StopId stop1Id, routing::transit::StopId stop2Id, routing::transit::LineId lineId,
                  std::vector<routing::transit::LineId> const & sameLines,
                  TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);
  void AddShape(TransitDisplayInfo const & transitDisplayInfo, routing::transit::StopId stop1Id,
                routing::transit::StopId stop2Id, routing::transit::LineId lineId, MwmSchemeData & scheme);

  void PrepareSchemeSubway(MwmSchemeData & scheme);
  void PrepareSchemePT(TransitDisplayInfo const & transitDisplayInfo, LinesDataPT const & lineData,
                       MwmSchemeData & scheme);

  void GenerateShapes(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId);

  void GenerateStops(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                     ref_ptr<dp::TextureManager> textures);

  void GenerateMarker(ref_ptr<dp::GraphicsContext> context, m2::PointD const & pt, m2::PointD widthDir,
                      float linesCountWidth, float linesCountHeight, float scaleWidth, float scaleHeight, float depth,
                      dp::Color const & color, dp::Batcher & batcher);

  void GenerateTransfer(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                        m2::PointD const & pivot, dp::Batcher & batcher);

  void GenerateTransfer(ref_ptr<dp::GraphicsContext> context, StopNodeParamsPT const & stopParams,
                        m2::PointD const & pivot, dp::Batcher & batcher);

  void GenerateStop(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                    m2::PointD const & pivot, std::map<routing::transit::LineId, LineParams> const & lines,
                    dp::Batcher & batcher);

  void GenerateStop(ref_ptr<dp::GraphicsContext> context, StopNodeParamsPT const & stopParams, m2::PointD const & pivot,
                    std::map<routing::transit::LineId, LineParams> const & lines, dp::Batcher & batcher);

  void GenerateTitles(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                      m2::PointD const & pivot, std::vector<m2::PointF> const & markerSizes,
                      ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher);

  void GenerateTitles(ref_ptr<dp::GraphicsContext> context, StopNodeParamsPT const & stopParams,
                      m2::PointD const & pivot, std::vector<m2::PointF> const & markerSizes,
                      ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher);

  void GenerateLine(ref_ptr<dp::GraphicsContext> context, std::vector<m2::PointD> const & path,
                    m2::PointD const & pivot, dp::Color const & colorConst, float lineOffset, float halfWidth,
                    float depth, dp::Batcher & batcher);

  StopNodeParamsPT & GetStopOrTransfer(MwmSchemeData & scheme, ::transit::TransitId id);

  using TransitSchemes = std::map<MwmSet::MwmId, MwmSchemeData>;
  TransitSchemes m_schemes;

  TFlushRenderDataFn m_flushRenderDataFn;

  uint32_t m_recacheId = 0;
};
}  // namespace df
