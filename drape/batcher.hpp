#pragma once

#include "drape/attribute_provider.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"
#include "drape/render_state.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "base/macros.hpp"

#include "std/map.hpp"
#include "std/function.hpp"

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

  void InsertTriangleList(RenderState const & state, ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleList(RenderState const & state, ref_ptr<AttributeProvider> params,
                                  drape_ptr<OverlayHandle> && handle);

  void InsertTriangleStrip(RenderState const & state, ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleStrip(RenderState const & state, ref_ptr<AttributeProvider> params,
                                   drape_ptr<OverlayHandle> && handle);

  void InsertTriangleFan(RenderState const & state, ref_ptr<AttributeProvider> params);
  IndicesRange InsertTriangleFan(RenderState const & state, ref_ptr<AttributeProvider> params,
                                 drape_ptr<OverlayHandle> && handle);

  void InsertListOfStrip(RenderState const & state, ref_ptr<AttributeProvider> params, uint8_t vertexStride);
  IndicesRange InsertListOfStrip(RenderState const & state, ref_ptr<AttributeProvider> params,
                                 drape_ptr<OverlayHandle> && handle, uint8_t vertexStride);

  void InsertLineStrip(RenderState const & state, ref_ptr<AttributeProvider> params);
  IndicesRange InsertLineStrip(RenderState const & state, ref_ptr<AttributeProvider> params,
                               drape_ptr<OverlayHandle> && handle);

  void InsertLineRaw(RenderState const & state, ref_ptr<AttributeProvider> params,
                     vector<int> const & indices);
  IndicesRange InsertLineRaw(RenderState const & state, ref_ptr<AttributeProvider> params,
                             vector<int> const & indices, drape_ptr<OverlayHandle> && handle);

  typedef function<void (RenderState const &, drape_ptr<RenderBucket> &&)> TFlushFn;
  void StartSession(TFlushFn const & flusher);
  void EndSession();
  void ResetSession();

  void SetFeatureMinZoom(int minZoom);

private:
  template<typename TBatcher, typename ... TArgs>
  IndicesRange InsertPrimitives(RenderState const & state, ref_ptr<AttributeProvider> params,
                                drape_ptr<OverlayHandle> && transferHandle, uint8_t vertexStride,
                                TArgs ... batcherArgs);

  class CallbacksWrapper;
  void ChangeBuffer(ref_ptr<CallbacksWrapper> wrapper);
  ref_ptr<RenderBucket> GetBucket(RenderState const & state);

  void FinalizeBucket(RenderState const & state);
  void Flush();

  TFlushFn m_flushInterface;

  using TBuckets = map<RenderState, drape_ptr<RenderBucket>>;
  TBuckets m_buckets;

  uint32_t m_indexBufferSize;
  uint32_t m_vertexBufferSize;

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
  SessionGuard(Batcher & batcher, Batcher::TFlushFn const & flusher);
  ~SessionGuard();

  DISALLOW_COPY_AND_MOVE(SessionGuard);
private:
  Batcher & m_batcher;
};
}  // namespace dp
