#include "drape/batcher.hpp"
#include "drape/batcher_helpers.hpp"
#include "drape/cpu_buffer.hpp"
#include "drape/index_storage.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <utility>

namespace dp
{
class Batcher::CallbacksWrapper : public BatchCallbacks
{
public:
  CallbacksWrapper(RenderState const & state, ref_ptr<OverlayHandle> overlay, ref_ptr<Batcher> batcher)
    : m_state(state)
    , m_overlay(overlay)
    , m_batcher(batcher)
  {}

  void SetVAO(ref_ptr<VertexArrayBuffer> buffer)
  {
    // Invocation with non-null VAO will cause to invalid range of indices.
    // It means that VAO has been changed during batching.
    if (m_buffer != nullptr)
      m_vaoChanged = true;

    m_buffer = buffer;
    m_indicesRange.m_idxStart = m_buffer->GetIndexCount();
  }

  void FlushData(ref_ptr<GraphicsContext> context, BindingInfo const & info, void const * data, uint32_t count) override
  {
    if (m_overlay != nullptr && info.IsDynamic())
    {
      uint32_t offset = m_buffer->GetDynamicBufferOffset(info);
      m_overlay->AddDynamicAttribute(info, offset, count);
    }
    m_buffer->UploadData(context, info, data, count);
  }

  void * GetIndexStorage(uint32_t size, uint32_t & startIndex) override
  {
    startIndex = m_buffer->GetStartIndexValue();
    if (m_overlay == nullptr || !m_overlay->IndexesRequired())
    {
      m_indexStorage.Resize(size);
      return m_indexStorage.GetRaw();
    }
    else
    {
      return m_overlay->IndexStorage(size);
    }
  }

  void SubmitIndices(ref_ptr<GraphicsContext> context) override
  {
    if (m_overlay == nullptr || !m_overlay->IndexesRequired())
      m_buffer->UploadIndices(context, m_indexStorage.GetRawConst(), m_indexStorage.Size());
  }

  uint32_t GetAvailableVertexCount() const override { return m_buffer->GetAvailableVertexCount(); }

  uint32_t GetAvailableIndexCount() const override { return m_buffer->GetAvailableIndexCount(); }

  void ChangeBuffer(ref_ptr<GraphicsContext> context) override { m_batcher->ChangeBuffer(context, make_ref(this)); }

  RenderState const & GetState() const { return m_state; }

  IndicesRange const & Finish()
  {
    if (!m_vaoChanged)
      m_indicesRange.m_idxCount = m_buffer->GetIndexCount() - m_indicesRange.m_idxStart;
    else
      m_indicesRange = IndicesRange();

    return m_indicesRange;
  }

private:
  RenderState const & m_state;
  ref_ptr<OverlayHandle> m_overlay;
  ref_ptr<Batcher> m_batcher;
  ref_ptr<VertexArrayBuffer> m_buffer;
  IndexStorage m_indexStorage;
  IndicesRange m_indicesRange;
  bool m_vaoChanged = false;
};

Batcher::Batcher(uint32_t indexBufferSize, uint32_t vertexBufferSize)
  : m_indexBufferSize(indexBufferSize)
  , m_vertexBufferSize(vertexBufferSize)
{}

Batcher::~Batcher()
{
  m_buckets.clear();
}

void Batcher::InsertTriangleList(ref_ptr<GraphicsContext> context, RenderState const & state,
                                 ref_ptr<AttributeProvider> params)
{
  InsertTriangleList(context, state, params, nullptr);
}

IndicesRange Batcher::InsertTriangleList(ref_ptr<GraphicsContext> context, RenderState const & state,
                                         ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle)
{
  return InsertPrimitives<TriangleListBatch>(context, state, params, std::move(handle), 0 /* vertexStride */);
}

void Batcher::InsertTriangleStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                  ref_ptr<AttributeProvider> params)
{
  InsertTriangleStrip(context, state, params, nullptr);
}

IndicesRange Batcher::InsertTriangleStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                          ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle)
{
  return InsertPrimitives<TriangleStripBatch>(context, state, params, std::move(handle), 0 /* vertexStride */);
}

void Batcher::InsertTriangleFan(ref_ptr<GraphicsContext> context, RenderState const & state,
                                ref_ptr<AttributeProvider> params)
{
  InsertTriangleFan(context, state, params, nullptr);
}

IndicesRange Batcher::InsertTriangleFan(ref_ptr<GraphicsContext> context, RenderState const & state,
                                        ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle)
{
  return InsertPrimitives<TriangleFanBatch>(context, state, params, std::move(handle), 0 /* vertexStride */);
}

void Batcher::InsertListOfStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                ref_ptr<AttributeProvider> params, uint8_t vertexStride)
{
  InsertListOfStrip(context, state, params, nullptr, vertexStride);
}

IndicesRange Batcher::InsertListOfStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                        ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle,
                                        uint8_t vertexStride)
{
  return InsertPrimitives<TriangleListOfStripBatch>(context, state, params, std::move(handle), vertexStride);
}

void Batcher::InsertLineStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                              ref_ptr<AttributeProvider> params)
{
  InsertLineStrip(context, state, params, nullptr);
}

IndicesRange Batcher::InsertLineStrip(ref_ptr<GraphicsContext> context, RenderState const & state,
                                      ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && handle)
{
  return InsertPrimitives<LineStripBatch>(context, state, params, std::move(handle), 0 /* vertexStride */);
}

void Batcher::InsertLineRaw(ref_ptr<GraphicsContext> context, RenderState const & state,
                            ref_ptr<AttributeProvider> params, std::vector<int> const & indices)
{
  InsertLineRaw(context, state, params, indices, nullptr);
}

IndicesRange Batcher::InsertLineRaw(ref_ptr<GraphicsContext> context, RenderState const & state,
                                    ref_ptr<AttributeProvider> params, std::vector<int> const & indices,
                                    drape_ptr<OverlayHandle> && handle)
{
  return InsertPrimitives<LineRawBatch>(context, state, params, std::move(handle), 0 /* vertexStride */, indices);
}

void Batcher::StartSession(TFlushFn && flusher)
{
  m_flushInterface = std::move(flusher);
}

void Batcher::EndSession(ref_ptr<GraphicsContext> context)
{
  Flush(context);
  m_flushInterface = TFlushFn();
}

void Batcher::ResetSession()
{
  m_flushInterface = TFlushFn();
  m_buckets.clear();
}

void Batcher::SetFeatureMinZoom(int minZoom)
{
  m_featureMinZoom = minZoom;

  for (auto const & bucket : m_buckets)
    bucket.second->SetFeatureMinZoom(m_featureMinZoom);
}

void Batcher::SetBatcherHash(uint64_t batcherHash)
{
  m_batcherHash = batcherHash;
}

void Batcher::ChangeBuffer(ref_ptr<GraphicsContext> context, ref_ptr<CallbacksWrapper> wrapper)
{
  RenderState const & state = wrapper->GetState();
  FinalizeBucket(context, state);

  CHECK(m_buckets.find(state) == m_buckets.end(), ());
  ref_ptr<RenderBucket> bucket = GetBucket(state);
  wrapper->SetVAO(bucket->GetBuffer());
}

ref_ptr<RenderBucket> Batcher::GetBucket(RenderState const & state)
{
  auto res = m_buckets.insert({state, nullptr});
  if (res.second)
  {
    drape_ptr<VertexArrayBuffer> vao =
        make_unique_dp<VertexArrayBuffer>(m_indexBufferSize, m_vertexBufferSize, m_batcherHash);
    drape_ptr<RenderBucket> buffer = make_unique_dp<RenderBucket>(std::move(vao));
    buffer->SetFeatureMinZoom(m_featureMinZoom);
    res.first->second = std::move(buffer);
  }

  return make_ref(res.first->second);
}

void Batcher::FinalizeBucket(ref_ptr<GraphicsContext> context, RenderState const & state)
{
  auto const it = m_buckets.find(state);
  CHECK(it != m_buckets.end(), ("Have no bucket for finalize with given state"));
  drape_ptr<RenderBucket> bucket = std::move(it->second);
  m_buckets.erase(it);

  bucket->GetBuffer()->Preflush(context);
  m_flushInterface(state, std::move(bucket));
}

void Batcher::Flush(ref_ptr<GraphicsContext> context)
{
  ASSERT(m_flushInterface != NULL, ());
  std::for_each(m_buckets.begin(), m_buckets.end(), [this, context](TBuckets::value_type & bucket)
  {
    ASSERT(bucket.second != nullptr, ());
    bucket.second->GetBuffer()->Preflush(context);
    m_flushInterface(bucket.first, std::move(bucket.second));
  });

  m_buckets.clear();
}

template <typename TBatcher, typename... TArgs>
IndicesRange Batcher::InsertPrimitives(ref_ptr<GraphicsContext> context, RenderState const & state,
                                       ref_ptr<AttributeProvider> params, drape_ptr<OverlayHandle> && transferHandle,
                                       uint8_t vertexStride, TArgs... batcherArgs)
{
  ref_ptr<VertexArrayBuffer> vao = GetBucket(state)->GetBuffer();
  IndicesRange range;

  drape_ptr<OverlayHandle> handle = std::move(transferHandle);

  {
    Batcher::CallbacksWrapper wrapper(state, make_ref(handle), make_ref(this));
    wrapper.SetVAO(vao);

    TBatcher batch(wrapper, batcherArgs...);
    batch.SetCanDivideStreams(handle == nullptr);
    batch.SetVertexStride(vertexStride);
    batch.BatchData(context, params);

    range = wrapper.Finish();
  }

  if (handle != nullptr)
    GetBucket(state)->AddOverlayHandle(std::move(handle));

  return range;
}

Batcher * BatcherFactory::GetNew() const
{
  return new Batcher(m_indexBufferSize, m_vertexBufferSize);
}

SessionGuard::SessionGuard(ref_ptr<GraphicsContext> context, Batcher & batcher, Batcher::TFlushFn && flusher)
  : m_context(context)
  , m_batcher(batcher)
{
  m_batcher.StartSession(std::move(flusher));
}

SessionGuard::~SessionGuard()
{
  m_batcher.EndSession(m_context);
}
}  // namespace dp
