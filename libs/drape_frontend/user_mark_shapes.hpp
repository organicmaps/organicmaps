#pragma once

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/user_marks_provider.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/spline.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace df
{
using MarksIDGroups = std::map<kml::MarkGroupId, drape_ptr<IDCollections>>;

/// Created and initialize in DrapeEngine::GenerateMarkRenderInfo only.
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
  DepthLayer m_titleDepthLayer = DepthLayer::UserMarkLayer;
  bool m_hasCreationAnimation = false;
  mutable bool m_justCreated = false;  ///< will be reset after first caching
  bool m_isVisible = true;
  FeatureID m_featureId;
  bool m_isMarkAboveText = false;
  float m_symbolOpacity = 1.0f;
  bool m_isSymbolSelectable = true;
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
using UserGroupsVisibilitySet = std::unordered_set<kml::MarkGroupId>;

class SourceBase
{
public:
  explicit SourceBase(UserGroupsVisibilitySet const * visibility) : m_visibility(visibility) {}
  void AddGroup(ref_ptr<MarksIDGroups> group) { m_groups.push_back(group); }
  bool IsEmpty() const { return m_groups.empty(); }

protected:
  template <class FnT>
  void ForEachVisibleGroup(FnT && fn) const
  {
    for (auto const & group : m_groups)
      for (auto const & [groupId, ids] : *group)
        if (m_visibility->contains(groupId))
          fn(*ids);
  }

private:
  UserGroupsVisibilitySet const * m_visibility;
  std::vector<ref_ptr<MarksIDGroups>> m_groups;
};

class MarksSource : public SourceBase
{
public:
  using SourceBase::SourceBase;

  /// Iterates marks across all visible groups, filtering by minZoom and tile containment.
  /// @param fn is called with UserMarkRenderParams const &.
  template <class FnT>
  void ForEachMark(TileKey const & tileKey, UserMarksRenderCollection const & marks, FnT && fn) const
  {
    auto const tileRect = tileKey.GetWrappedDataRect();
    ForEachVisibleGroup([&](IDCollections const & ids)
    {
      for (auto const markId : ids.m_markIds)
      {
        auto it = marks.find(markId);
        if (it == marks.end())
          continue;

        auto const & rp = *it->second;
        if (rp.m_isVisible && rp.m_minZoom <= tileKey.m_zoomLevel && tileRect.IsPointInside(rp.m_pivot))
          fn(rp);
      }
    });
  }
};

class TracksSource : public SourceBase
{
public:
  using SourceBase::SourceBase;

  /// Iterates unique tracks across all visible groups, filtering by minZoom.
  /// @param fn is called with UserLineRenderParams const &.
  template <class FnT>
  void ForEachUniqueTrack(int zoom, UserLinesRenderCollection const & lines, FnT && fn) const
  {
    std::unordered_set<kml::TrackId> visited;
    ForEachVisibleGroup([&](IDCollections const & ids)
    {
      for (auto const lineId : ids.m_lineIds)
        if (visited.insert(lineId).second)
        {
          auto it = lines.find(lineId);
          if (it != lines.end() && it->second->m_minZoom <= zoom)
            fn(*it->second);
        }
    });
  }
};

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

void CacheUserMarks(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    MarksSource const & source, UserMarksRenderCollection const & renderParams, dp::Batcher & batcher);

void CacheUserLines(ref_ptr<dp::GraphicsContext> context, TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    TracksSource const & source, UserLinesRenderCollection const & renderParams, dp::Batcher & batcher);
}  // namespace df
