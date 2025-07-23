#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/spline.hpp"

#include <limits>
#include <memory>
#include <unordered_map>

namespace df
{
struct UserMarkRenderParams
{
  kml::MarkId m_markId = kml::kInvalidMarkId;
  int m_minZoom = 1;
  int m_minTitleZoom = 1;
  m2::PointD m_pivot = m2::PointD(0.0, 0.0);
  m2::PointD m_pixelOffset = m2::PointD(0.0, 0.0);
  dp::Anchor m_anchor = dp::Center;
  drape_ptr<UserPointMark::ColoredSymbolZoomInfo> m_coloredSymbols;
  drape_ptr<UserPointMark::SymbolNameZoomInfo> m_symbolNames;
  drape_ptr<UserPointMark::TitlesInfo> m_titleDecl;
  drape_ptr<UserPointMark::SymbolSizes> m_symbolSizes;
  drape_ptr<UserPointMark::SymbolOffsets> m_symbolOffsets;
  df::ColorConstant m_color;
  bool m_symbolIsPOI = false;
  bool m_hasTitlePriority = false;
  uint16_t m_priority = 0;
  SpecialDisplacement m_displacement = SpecialDisplacement::UserMark;
  uint32_t m_index = 0;
  bool m_depthTestEnabled = true;
  float m_depth = 0.0;
  bool m_customDepth = false;
  DepthLayer m_depthLayer = DepthLayer::UserMarkLayer;
  bool m_hasCreationAnimation = false;
  mutable bool m_justCreated = false;  ///< will be reset after first caching
  bool m_isVisible = true;
  FeatureID m_featureId;
  bool m_isMarkAboveText = false;
  float m_symbolOpacity = 1.0f;
  bool m_isSymbolSelectable = true;
  bool m_isNonDisplaceable = false;
};

struct LineLayer
{
  LineLayer() = default;
  LineLayer(dp::Color color, float width, float depth) : m_color(color), m_width(width), m_depth(depth) {}

  dp::Color m_color;
  float m_width = 0.0;
  float m_depth = 0.0;
};

struct UserLineRenderParams
{
  int m_minZoom = 1;
  DepthLayer m_depthLayer = DepthLayer::UserLineLayer;
  std::vector<LineLayer> m_layers;
  std::vector<m2::SharedSpline> m_splines;
};

using UserMarksRenderCollection = std::unordered_map<kml::MarkId, drape_ptr<UserMarkRenderParams>>;
using UserLinesRenderCollection = std::unordered_map<kml::MarkId, drape_ptr<UserLineRenderParams>>;

struct UserMarkRenderData
{
  UserMarkRenderData(dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket, TileKey const & tileKey)
    : m_state(state)
    , m_bucket(std::move(bucket))
    , m_tileKey(tileKey)
  {}

  dp::RenderState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  TileKey m_tileKey;
};

using TUserMarksRenderData = std::vector<UserMarkRenderData>;

void ProcessSplineSegmentRects(m2::SharedSpline const & spline, double maxSegmentLength,
                               std::function<bool(m2::RectD const & segmentRect)> const & func);

void CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    kml::MarkIdCollection const & marksId, UserMarksRenderCollection const & renderParams,
                    dp::Batcher & batcher);

void CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    kml::TrackIdCollection const & linesId, UserLinesRenderCollection const & renderParams,
                    dp::Batcher & batcher);
}  // namespace df
