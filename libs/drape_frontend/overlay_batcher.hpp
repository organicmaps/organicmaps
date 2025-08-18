#pragma once

#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/batcher.hpp"
#include "drape/pointers.hpp"

#include <utility>
#include <vector>

namespace dp
{
class TextureManager;
class RenderBucket;
}  // namespace dp

namespace df
{
class MapShape;

struct OverlayRenderData
{
  TileKey m_tileKey;
  dp::RenderState m_state;
  drape_ptr<dp::RenderBucket> m_bucket;

  OverlayRenderData(TileKey const & key, dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket)
    : m_tileKey(key)
    , m_state(state)
    , m_bucket(std::move(bucket))
  {}
};

using TOverlaysRenderData = std::vector<OverlayRenderData>;

class OverlayBatcher
{
public:
  explicit OverlayBatcher(TileKey const & key);
  void Batch(ref_ptr<dp::GraphicsContext> context, drape_ptr<MapShape> const & shape,
             ref_ptr<dp::TextureManager> texMng);
  void Finish(ref_ptr<dp::GraphicsContext> context, TOverlaysRenderData & data);

private:
  void FlushGeometry(TileKey const & key, dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket);

  dp::Batcher m_batcher;
  TOverlaysRenderData m_data;
};
}  // namespace df
