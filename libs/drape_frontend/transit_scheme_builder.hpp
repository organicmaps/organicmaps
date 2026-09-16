#pragma once

#include "drape_frontend/selection_info.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/texture_manager.hpp"

#include "transit/transit_display_info.hpp"

#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace df
{
float constexpr kTransitLineHalfWidth = 0.64f;
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

  /// Builds render data for a single relation's transit view (lines + stops markers).
  /// @param mwmId Always empty for now — used as a sentinel bucket key so Clear(MwmId{}) can drop it later.
  /// Implementation must not touch the m_schemes cache used by the mwm-scale UpdateSchemes path.
  void BuildFromRouteTransit(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                             TransitInfo const & info, ref_ptr<dp::TextureManager> textures);

private:
  struct MwmSchemeData
  {
    m2::PointD m_pivot;

    std::map<routing::transit::LineId, LineParams> m_linesSubway;
    std::map<routing::transit::ShapeId, ShapeParams> m_shapesSubway;
    std::map<routing::transit::StopId, StopNodeParamsSubway> m_stopsSubway;
    std::map<routing::transit::TransferId, StopNodeParamsSubway> m_transfersSubway;
  };

  void BuildScheme(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                   ref_ptr<dp::TextureManager> textures);

  void CollectStopsSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSet::MwmId const & mwmId,
                          MwmSchemeData & scheme);
  void CollectLinesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);

  void CollectShapesSubway(TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);

  void FindShapes(routing::transit::StopId stop1Id, routing::transit::StopId stop2Id, routing::transit::LineId lineId,
                  std::vector<routing::transit::LineId> const & sameLines,
                  TransitDisplayInfo const & transitDisplayInfo, MwmSchemeData & scheme);
  void AddShape(TransitDisplayInfo const & transitDisplayInfo, routing::transit::StopId stop1Id,
                routing::transit::StopId stop2Id, routing::transit::LineId lineId, MwmSchemeData & scheme);

  void PrepareSchemeSubway(MwmSchemeData & scheme);

  void GenerateLines(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId);

  void GenerateStops(ref_ptr<dp::GraphicsContext> context, MwmSet::MwmId const & mwmId,
                     ref_ptr<dp::TextureManager> textures);

  void GenerateMarker(ref_ptr<dp::GraphicsContext> context, m2::PointD const & pt, m2::PointD widthDir,
                      float linesCountWidth, float linesCountHeight, float scaleWidth, float scaleHeight, float depth,
                      dp::Color const & color, dp::Batcher & batcher);

  void GenerateTransfer(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                        m2::PointD const & pivot, dp::Batcher & batcher);

  void GenerateStop(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                    m2::PointD const & pivot, std::map<routing::transit::LineId, LineParams> const & lines,
                    dp::Batcher & batcher);

  void GenerateTitles(ref_ptr<dp::GraphicsContext> context, StopNodeParamsSubway const & stopParams,
                      m2::PointD const & pivot, std::vector<m2::PointF> const & markerSizes,
                      ref_ptr<dp::TextureManager> textures, dp::Batcher & batcher);

  void GenerateLine(ref_ptr<dp::GraphicsContext> context, std::vector<m2::PointD> const & path,
                    m2::PointD const & pivot, dp::Color const & colorConst, float lineOffset, float halfWidth,
                    float depth, dp::Batcher & batcher);

  using TransitSchemes = std::map<MwmSet::MwmId, MwmSchemeData>;
  TransitSchemes m_schemes;

  TFlushRenderDataFn m_flushRenderDataFn;

  uint32_t m_recacheId = 0;
};
}  // namespace df
