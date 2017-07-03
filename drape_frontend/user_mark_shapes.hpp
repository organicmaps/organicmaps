#pragma once

#include "tile_key.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/spline.hpp"

#include <memory>

namespace df
{
struct UserMarkRenderParams
{
  int m_minZoom = 1;
  m2::PointD m_pivot = m2::PointD(0.0, 0.0);
  m2::PointD m_pixelOffset = m2::PointD(0.0, 0.0);
  std::string m_symbolName;
  dp::Anchor m_anchor = dp::Center;
  float m_depth = 0.0;
  bool m_runCreationAnim = false;
  bool m_isVisible = true;
};

struct LineLayer
{
  LineLayer() = default;
  LineLayer(dp::Color color, float width, float depth)
    : m_color(color)
    , m_width(width)
    , m_depth(depth)
  {}

  dp::Color m_color;
  float m_width = 0.0;
  float m_depth = 0.0;
};

struct UserLineRenderParams
{
  int m_minZoom = 1;
  std::vector<LineLayer> m_layers;
  m2::SharedSpline m_spline;
};

using UserMarksRenderCollection = std::vector<UserMarkRenderParams>;
using UserLinesRenderCollection = std::vector<UserLineRenderParams>;

using MarkIndexesCollection = std::vector<uint32_t>;
using LineIndexesCollection = std::vector<uint32_t>;

struct UserMarkRenderData
{
  UserMarkRenderData(dp::GLState const & state,
                     drape_ptr<dp::RenderBucket> && bucket,
                     TileKey const & tileKey)
    : m_state(state), m_bucket(move(bucket)), m_tileKey(tileKey)
  {}

  dp::GLState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;
  TileKey m_tileKey;
};

using TUserMarksRenderData = std::vector<UserMarkRenderData>;

void CacheUserMarks(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    MarkIndexesCollection const & indexes, UserMarksRenderCollection & renderParams,
                    dp::Batcher & batcher);

void CacheUserLines(TileKey const & tileKey, ref_ptr<dp::TextureManager> textures,
                    LineIndexesCollection const & indexes, UserLinesRenderCollection & renderParams,
                    dp::Batcher & batcher);
}  // namespace df
