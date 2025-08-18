#pragma once

#include "drape/attribute_provider.hpp"
#include "drape/graphics_context.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "base/macros.hpp"

#include <functional>
#include <map>
#include <vector>

namespace dp
{
class RenderBucket;
class AttributeProvider;
class OverlayHandle;

class Batcher
{
public:
  static uint32_t const IndexPerTriangle = 3;
  static uint32_t const IndexPerQuad = 6;
  static uint32_t const VertexPerQuad = 4;

  Batcher(uint32_t indexBufferSize, uint32_t vertexBufferSize);
  ~Batcher();

  void InsertTriangleList(ref_ptr<GraphicsContext> context, RenderState const & state,
                          ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleList(ref_ptr<GraphicsContext> context, RenderState const & state,
                                  ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle);

  void InsertTriangleStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                           ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                   ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle);

  void InsertTriangleFan(ref_ptr<GraphicsContext> context, RenderState const & state,
                         ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleFan(ref_ptr<GraphicsContext> context, RenderState const & state,
                                 ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle);

  void InsertListOfStrip(ref_ptr<GraphicsContext> context, RenderState const & state, ref_ptr<AttributeProvider> params,
                         uint8_t vertexStride);
  IndicesRange InsertListOfStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                 ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle,
                                 uint8_t vertexStride);

  void InsertLineStrip(ref_ptr<GraphicsContext> context, RenderState const & state, ref_ptr<AttributeProvider> params);
  IndicesRange InsertLineStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                               ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle);

  void InsertLineRaw(ref_ptr<GraphicsContext> context, RenderState const & state, ref_ptr<AttributeProvider> params,
                     std::vector<int> const & indices);
  IndicesRange InsertLineRaw(ref_ptr<GraphicsContext> context, RenderState const & state,
                             ref_ptr<AttributeProvider> params, std::vector<int> const & indices,
                             drape_ptr<OverlayHandle> && handle);

  using TFlushFn = std::function<void(RenderState const &, drape_ptr<RenderBucket> &&)>;
  void StartSession(TFlushFn && flusher);
  void EndSession(ref_ptr<GraphicsContext> context);
  void ResetSession();

  void SetBatcherHash(uint64_t batcherHash);

  void SetFeatureMinZoom(int minZoom);

private:
  template <typename TBatcher, typename... TArgs>
  IndicesRange InsertPrimitives(ref_ptr<GraphicsContext> context, RenderState const & state,
                                ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && transferHandle,
                                uint8_t vertexStride, TArgs... batcherArgs);

  class CallbacksWrapper;
  void ChangeBuffer(ref_ptr<GraphicsContext> context, ref_ptr<CallbacksWrapper> wrapper);
  ref_ptr<RenderBucket> GetBucket(RenderState const & state);

  void FinalizeBucket(ref_ptr<GraphicsContext> context, RenderState const & state);
  void Flush(ref_ptr<GraphicsContext> context);

  uint32_t const m_indexBufferSize;
  uint32_t const m_vertexBufferSize;

  uint64_t m_batcherHash = 0;

  TFlushFn m_flushInterface;

  using TBuckets = std::map<RenderState, drape_ptr<RenderBucket>>;
  TBuckets m_buckets;

  int m_featureMinZoom = 0;
};

class BatcherFactory
{
public:
  BatcherFactory(uint32_t indexBufferSize, uint32_t vertexBufferSize)
    : m_indexBufferSize(indexBufferSize)
    , m_vertexBufferSize(vertexBufferSize)
  {}

  Batcher * GetNew() const;

private:
  uint32_t const m_indexBufferSize;
  uint32_t const m_vertexBufferSize;
};

class SessionGuard
{
public:
  SessionGuard(ref_ptr<GraphicsContext> context, Batcher & batcher, Batcher::TFlushFn && flusher);
  ~SessionGuard();

private:
  ref_ptr<GraphicsContext> m_context;
  Batcher & m_batcher;
  DISALLOW_COPY_AND_MOVE(SessionGuard);
};
}  // namespace dp
