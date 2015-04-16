#include "drape/batcher.hpp"
#include "drape/cpu_buffer.hpp"
#include "drape/batcher_helpers.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"

namespace dp
{

class Batcher::CallbacksWrapper
{
public:
  CallbacksWrapper(GLState const & state, ref_ptr<OverlayHandle> overlay)
    : m_state(state)
    , m_overlay(overlay)
    , m_vaoChanged(false)
  {
  }

  void SetVAO(ref_ptr<VertexArrayBuffer> buffer)
  {
    // invocation with non-null VAO will cause to invalid range of indices.
    // It means that VAO has been changed during batching
    if (!m_buffer.IsNull())
      m_vaoChanged = true;

    m_buffer = buffer;
    m_indicesRange.m_idxStart = m_buffer->GetIndexCount();
  }

  bool IsVAOFilled() const
  {
    return m_buffer->IsFilled();
  }

  void FlushData(BindingInfo const & info, void const * data, uint16_t count)
  {
    if (m_overlay != nullptr && info.IsDynamic())
    {
      uint16_t offset = m_buffer->GetDynamicBufferOffset(info);
      m_overlay->AddDynamicAttribute(info, offset, count);
    }
    m_buffer->UploadData(info, data, count);
  }

  uint16_t * GetIndexStorage(uint16_t size, uint16_t & startIndex)
  {
    startIndex = m_buffer->GetStartIndexValue();
    if (m_overlay == nullptr || !m_overlay->IndexesRequired())
    {
      m_indexStorage.resize(size);
      return &m_indexStorage[0];
    }
    else
      return m_overlay->IndexStorage(size);
  }

  void SubmitIndexes()
  {
    if (m_overlay == nullptr || !m_overlay->IndexesRequired())
      m_buffer->UploadIndexes(&m_indexStorage[0], m_indexStorage.size());
  }

  uint16_t GetAvailableVertexCount() const
  {
    return m_buffer->GetAvailableVertexCount();
  }

  uint16_t GetAvailableIndexCount() const
  {
    return m_buffer->GetAvailableIndexCount();
  }

  GLState const & GetState() const
  {
    return m_state;
  }

  IndicesRange const & Finish()
  {
    if (!m_vaoChanged)
      m_indicesRange.m_idxCount = m_buffer->GetIndexCount() - m_indicesRange.m_idxStart;
    else
      m_indicesRange = IndicesRange();

    return m_indicesRange;
  }

private:
  GLState const & m_state;
  ref_ptr<VertexArrayBuffer> m_buffer;
  ref_ptr<OverlayHandle> m_overlay;
  vector<uint16_t> m_indexStorage;
  IndicesRange m_indicesRange;
  bool m_vaoChanged;
};

////////////////////////////////////////////////////////////////

Batcher::Batcher(uint32_t indexBufferSize, uint32_t vertexBufferSize)
  : m_indexBufferSize(indexBufferSize)
  , m_vertexBufferSize(vertexBufferSize)
{
}

Batcher::~Batcher()
{
  m_buckets.clear();
}

IndicesRange Batcher::InsertTriangleList(GLState const & state, ref_ptr<AttributeProvider> params)
{
  return InsertTriangleList(state, params, drape_ptr<OverlayHandle>(nullptr));
}

IndicesRange Batcher::InsertTriangleList(GLState const & state, ref_ptr<AttributeProvider> params,
                                         drape_ptr<OverlayHandle> && handle)
{
  return InsertTriangles<TriangleListBatch>(state, params, move(handle));
}

IndicesRange Batcher::InsertTriangleStrip(GLState const & state, ref_ptr<AttributeProvider> params)
{
  return InsertTriangleStrip(state, params, drape_ptr<OverlayHandle>(nullptr));
}

IndicesRange Batcher::InsertTriangleStrip(GLState const & state, ref_ptr<AttributeProvider> params,
                                          drape_ptr<OverlayHandle> && handle)
{
  return InsertTriangles<TriangleStripBatch>(state, params, move(handle));
}

IndicesRange Batcher::InsertTriangleFan(GLState const & state, ref_ptr<AttributeProvider> params)
{
  return InsertTriangleFan(state, params, drape_ptr<OverlayHandle>(nullptr));
}

IndicesRange Batcher::InsertTriangleFan(GLState const & state, ref_ptr<AttributeProvider> params,
                                        drape_ptr<OverlayHandle> && handle)
{
  return InsertTriangles<TriangleFanBatch>(state, params, move(handle));
}

IndicesRange Batcher::InsertListOfStrip(GLState const & state, ref_ptr<AttributeProvider> params,
                                        uint8_t vertexStride)
{
  return InsertListOfStrip(state, params, drape_ptr<OverlayHandle>(nullptr), vertexStride);
}

IndicesRange Batcher::InsertListOfStrip(GLState const & state, ref_ptr<AttributeProvider> params,
                                        drape_ptr<OverlayHandle> && handle, uint8_t vertexStride)
{
  return InsertTriangles<TriangleListOfStripBatch>(state, params, move(handle), vertexStride);
}

void Batcher::StartSession(TFlushFn const & flusher)
{
  m_flushInterface = flusher;
}

void Batcher::EndSession()
{
  Flush();
  m_flushInterface = TFlushFn();
}

void Batcher::ChangeBuffer(ref_ptr<CallbacksWrapper> wrapper, bool checkFilledBuffer)
{
  if (wrapper->IsVAOFilled() || checkFilledBuffer == false)
  {
    GLState const & state = wrapper->GetState();
    FinalizeBucket(state);

    ref_ptr<RenderBucket> bucket = GetBucket(state);
    wrapper->SetVAO(bucket->GetBuffer());
  }
}

ref_ptr<RenderBucket> Batcher::GetBucket(GLState const & state)
{
  TBuckets::iterator it = m_buckets.find(state);
  if (it != m_buckets.end())
    return make_ref<RenderBucket>(it->second);

  drape_ptr<VertexArrayBuffer> vao = make_unique_dp<VertexArrayBuffer>(m_indexBufferSize, m_vertexBufferSize);
  drape_ptr<RenderBucket> buffer = make_unique_dp<RenderBucket>(move(vao));
  ref_ptr<RenderBucket> result = make_ref<RenderBucket>(buffer);
  m_buckets.emplace(state, move(buffer));

  return result;
}

void Batcher::FinalizeBucket(GLState const & state)
{
  TBuckets::iterator it = m_buckets.find(state);
  ASSERT(it != m_buckets.end(), ("Have no bucket for finalize with given state"));
  drape_ptr<RenderBucket> bucket = move(it->second);
  m_buckets.erase(state);
  bucket->GetBuffer()->Preflush();
  m_flushInterface(state, move(bucket));
}

void Batcher::Flush()
{
  ASSERT(m_flushInterface != NULL, ());
  for_each(m_buckets.begin(), m_buckets.end(), [this](TBuckets::value_type & bucket)
  {
    ASSERT(bucket.second != nullptr, ());
    bucket.second->GetBuffer()->Preflush();
    m_flushInterface(bucket.first, move(bucket.second));
  });

  m_buckets.clear();
}

template <typename TBatcher>
IndicesRange Batcher::InsertTriangles(GLState const & state, ref_ptr<AttributeProvider> params,
                                      drape_ptr<OverlayHandle> && transferHandle, uint8_t vertexStride)
{
  ref_ptr<RenderBucket> bucket = GetBucket(state);
  ref_ptr<VertexArrayBuffer> vao = bucket->GetBuffer();
  IndicesRange range;

  drape_ptr<OverlayHandle> handle = move(transferHandle);

  {
    Batcher::CallbacksWrapper wrapper(state, make_ref<OverlayHandle>(handle));
    wrapper.SetVAO(vao);

    BatchCallbacks callbacks;
    callbacks.m_flushVertex = bind(&CallbacksWrapper::FlushData, &wrapper, _1, _2, _3);
    callbacks.m_getIndexStorage = bind(&CallbacksWrapper::GetIndexStorage, &wrapper, _1, _2);
    callbacks.m_submitIndex = bind(&CallbacksWrapper::SubmitIndexes, &wrapper);
    callbacks.m_getAvailableVertex = bind(&CallbacksWrapper::GetAvailableVertexCount, &wrapper);
    callbacks.m_getAvailableIndex = bind(&CallbacksWrapper::GetAvailableIndexCount, &wrapper);
    callbacks.m_changeBuffer = bind(&Batcher::ChangeBuffer, this, make_ref(&wrapper), _1);

    TBatcher batch(callbacks);
    batch.SetIsCanDevideStreams(handle == nullptr);
    batch.SetVertexStride(vertexStride);
    batch.BatchData(params);

    range = wrapper.Finish();
  }

  if (handle != nullptr)
    bucket->AddOverlayHandle(move(handle));

  return range;
}

SessionGuard::SessionGuard(Batcher & batcher, const Batcher::TFlushFn & flusher)
  : m_batcher(batcher)
{
  m_batcher.StartSession(flusher);
}

SessionGuard::~SessionGuard()
{
  m_batcher.EndSession();
}

} // namespace dp
