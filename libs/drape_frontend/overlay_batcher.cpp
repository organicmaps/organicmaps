#include "drape_frontend/overlay_batcher.hpp"

#include "drape_frontend/map_shape.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"
#include "drape/texture_manager.hpp"

namespace df
{
uint32_t const kOverlayIndexBufferSize = 30000;
uint32_t const kOverlayVertexBufferSize = 20000;

OverlayBatcher::OverlayBatcher(TileKey const & key) : m_batcher(kOverlayIndexBufferSize, kOverlayVertexBufferSize)
{
  int const kAverageRenderDataCount = 5;
  m_data.reserve(kAverageRenderDataCount);

  m_batcher.SetBatcherHash(key.GetHashValue(BatcherBucket::Overlay));
  m_batcher.StartSession([this, key](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && bucket)
  { FlushGeometry(key, state, std::move(bucket)); });
}

void OverlayBatcher::Batch(ref_ptr<dp::GraphicsContext> context, drape_ptr<MapShape> const & shape,
                           ref_ptr<dp::TextureManager> texMng)
{
  m_batcher.SetFeatureMinZoom(shape->GetFeatureMinZoom());
  shape->Draw(context, make_ref(&m_batcher), texMng);
}

void OverlayBatcher::Finish(ref_ptr<dp::GraphicsContext> context, TOverlaysRenderData & data)
{
  m_batcher.EndSession(context);
  data.swap(m_data);
}

void OverlayBatcher::FlushGeometry(TileKey const & key, dp::RenderState const & state,
                                   drape_ptr<dp::RenderBucket> && bucket)
{
  m_data.emplace_back(key, state, std::move(bucket));
}
}  // namespace df
