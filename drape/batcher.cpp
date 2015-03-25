#include "drape/batcher.hpp"
#include "drape/cpu_buffer.hpp"
#include "drape/batcher_helpers.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "base/assert.hpp"

#include "std/bind.hpp"

namespace dp
{

class Batcher::CallbacksWrapper
{
public:
  CallbacksWrapper(GLState const & state, RefPointer<OverlayHandle> overlay)
    : m_state(state)
    , m_overlay(overlay)
  {

  }

  void SetVAO(RefPointer<VertexArrayBuffer> buffer)
  {
    m_buffer = buffer;
  }

  bool IsVAOFilled() const
  {
    return m_buffer->IsFilled();
  }

  void FlushData(BindingInfo const & info, void const * data, uint16_t count)
  {
    if (!m_overlay.IsNull() && info.IsDynamic())
    {
      uint16_t offset = m_buffer->GetDynamicBufferOffset(info);
      m_overlay->AddDynamicAttribute(info, offset, count);
    }
    m_buffer->UploadData(info, data, count);
  }

  uint16_t * GetIndexStorage(uint16_t size, uint16_t & startIndex)
  {
    startIndex = m_buffer->GetStartIndexValue();
    if (m_overlay.IsNull() || !m_overlay->IndexesRequired())
    {
      m_indexStorage.resize(size);
      return &m_indexStorage[0];
    }
    else
      return m_overlay->IndexStorage(size);
  }

  void SubmitIndexes()
  {
    if (m_overlay.IsNull() || !m_overlay->IndexesRequired())
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

private:
  GLState const & m_state;
  RefPointer<VertexArrayBuffer> m_buffer;
  RefPointer<OverlayHandle>     m_overlay;
  vector<uint16_t>              m_indexStorage;
};

////////////////////////////////////////////////////////////////

Batcher::Batcher(uint32_t indexBufferSize, uint32_t vertexBufferSize)
  : m_indexBufferSize(indexBufferSize)
  , m_vertexBufferSize(vertexBufferSize)
{
}

Batcher::~Batcher()
{
  buckets_t::iterator it = m_buckets.begin();
  for (; it != m_buckets.end(); ++it)
    it->second.Destroy();
}

void Batcher::InsertTriangleList(GLState const & state, RefPointer<AttributeProvider> params)
{
  InsertTriangleList(state, params, MovePointer<OverlayHandle>(NULL));
}

void Batcher::InsertTriangleList(GLState const & state, RefPointer<AttributeProvider> params,
                                 TransferPointer<OverlayHandle> handle)
{
  InsertTriangles<TriangleListBatch>(state, params, handle);
}

void Batcher::InsertTriangleStrip(GLState const & state, RefPointer<AttributeProvider> params)
{
  InsertTriangleStrip(state, params, MovePointer<OverlayHandle>(NULL));
}

void Batcher::InsertTriangleStrip(GLState const & state, RefPointer<AttributeProvider> params,
                                  TransferPointer<OverlayHandle> handle)
{
  InsertTriangles<TriangleStripBatch>(state, params, handle);
}

void Batcher::InsertTriangleFan(GLState const & state, RefPointer<AttributeProvider> params)
{
  InsertTriangleFan(state, params, MovePointer<OverlayHandle>(NULL));
}

void Batcher::InsertTriangleFan(GLState const & state, RefPointer<AttributeProvider> params,
                                TransferPointer<OverlayHandle> handle)
{
  InsertTriangles<TriangleFanBatch>(state, params, handle);
}

void Batcher::InsertListOfStrip(GLState const & state, RefPointer<AttributeProvider> params, uint8_t vertexStride)
{
  InsertListOfStrip(state, params, MovePointer<OverlayHandle>(NULL), vertexStride);
}

void Batcher::InsertListOfStrip(GLState const & state, RefPointer<AttributeProvider> params,
                       TransferPointer<OverlayHandle> handle, uint8_t vertexStride)
{
  InsertTriangles<TriangleListOfStripBatch>(state, params, handle, vertexStride);
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

void Batcher::ChangeBuffer(RefPointer<CallbacksWrapper> wrapper, bool checkFilledBuffer)
{
  if (wrapper->IsVAOFilled() || checkFilledBuffer == false)
  {
    GLState const & state = wrapper->GetState();
    FinalizeBucket(state);

    RefPointer<RenderBucket> bucket = GetBucket(state);
    wrapper->SetVAO(bucket->GetBuffer());
  }
}

RefPointer<RenderBucket> Batcher::GetBucket(GLState const & state)
{
  buckets_t::iterator it = m_buckets.find(state);
  if (it != m_buckets.end())
    return it->second.GetRefPointer();

  MasterPointer<VertexArrayBuffer> vao(new VertexArrayBuffer(m_indexBufferSize, m_vertexBufferSize));
  MasterPointer<RenderBucket> buffer(new RenderBucket(vao.Move()));
  m_buckets.insert(make_pair(state, buffer));
  return buffer.GetRefPointer();
}

void Batcher::FinalizeBucket(GLState const & state)
{
  ASSERT(m_buckets.find(state) != m_buckets.end(), ("Have no bucket for finalize with given state"));
  MasterPointer<RenderBucket> bucket = m_buckets[state];
  m_buckets.erase(state);
  bucket->GetBuffer()->Preflush();
  m_flushInterface(state, bucket.Move());
}

void Batcher::Flush()
{
  ASSERT(m_flushInterface != NULL, ());
  for (buckets_t::iterator it = m_buckets.begin(); it != m_buckets.end(); ++it)
  {
    it->second->GetBuffer()->Preflush();
    m_flushInterface(it->first, it->second.Move());
  }

  m_buckets.clear();
}

template <typename TBatcher>
void Batcher::InsertTriangles(GLState const & state,
                              RefPointer<AttributeProvider> params,
                              TransferPointer<OverlayHandle> transferHandle,
                              uint8_t vertexStride)
{
  RefPointer<RenderBucket> bucket = GetBucket(state);
  RefPointer<VertexArrayBuffer> vao = bucket->GetBuffer();

  MasterPointer<OverlayHandle> handle(transferHandle);

  {
    Batcher::CallbacksWrapper wrapper(state, handle.GetRefPointer());
    wrapper.SetVAO(vao);

    BatchCallbacks callbacks;
    callbacks.m_flushVertex = bind(&CallbacksWrapper::FlushData, &wrapper, _1, _2, _3);
    callbacks.m_getIndexStorage = bind(&CallbacksWrapper::GetIndexStorage, &wrapper, _1, _2);
    callbacks.m_submitIndex = bind(&CallbacksWrapper::SubmitIndexes, &wrapper);
    callbacks.m_getAvailableVertex = bind(&CallbacksWrapper::GetAvailableVertexCount, &wrapper);
    callbacks.m_getAvailableIndex = bind(&CallbacksWrapper::GetAvailableIndexCount, &wrapper);
    callbacks.m_changeBuffer = bind(&Batcher::ChangeBuffer, this, MakeStackRefPointer(&wrapper), _1);

    TBatcher batch(callbacks);
    batch.SetIsCanDevideStreams(handle.IsNull());
    batch.SetVertexStride(vertexStride);
    batch.BatchData(params);
  }

  if (!handle.IsNull())
    bucket->AddOverlayHandle(handle.Move());
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
