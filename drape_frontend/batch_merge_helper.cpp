#include "batch_merge_helper.hpp"

#include "drape/glextensions_list.hpp"
#include "drape/render_bucket.hpp"
#include "drape/vertex_array_buffer.hpp"

namespace df
{

void ReadBufferData(void * dst, glConst target, uint32_t size)
{
#ifdef OMIM_OS_DESKTOP
  void * bufferPointer = GLFunctions::glMapBuffer(target, gl_const::GLReadOnly);
#else
  void * bufferPointer = GLFunctions::glMapBufferRange(target, 0, size, gl_const::GLReadBufferBit);
#endif

  memcpy(dst, bufferPointer, size);
}

struct NotSupported32BitIndex{};
struct Supported32BitIndex{};

template<typename TSupported>
void TransformIndeces(void * pointer, uint32_t count, uint32_t offset)
{
  ASSERT(false, ());
}

template<>
void TransformIndeces<Supported32BitIndex>(void * pointer, uint32_t count, uint32_t offset)
{
  uint32_t * indexPtr = reinterpret_cast<uint32_t *>(pointer);
  for (uint32_t i = 0; i < count; ++i)
  {
    *indexPtr += offset;
    ++indexPtr;
  }
}

template<>
void TransformIndeces<NotSupported32BitIndex>(void * pointer, uint32_t count, uint32_t offset)
{
  uint16_t * indexPtr = reinterpret_cast<uint16_t *>(pointer);
  uint16_t indexOffset = static_cast<uint16_t>(offset);
  for (uint32_t i = 0; i < count; ++i)
  {
    *indexPtr += indexOffset;
    ++indexPtr;
  }
}

bool BatchMergeHelper::IsMergeSupported()
{
#if defined(OMIM_OS_DESKTOP)
  return true;
#else
  static bool isSupported = dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::MapBufferRange);
  return isSupported;
#endif
}

void BatchMergeHelper::MergeBatches(vector<drape_ptr<RenderGroup>> & batches,
                                    vector<drape_ptr<RenderGroup>> & mergedBatches)
{
  ASSERT(!batches.empty(), ());
  if (batches.size() < 2)
  {
    mergedBatches.emplace_back(move(batches.front()));
    return;
  }

  uint32_t const kIndexBufferSize = 30000;
  uint32_t const kVertexBufferSize = 20000;

  using TBuffer = dp::VertexArrayBuffer;
  using TBucket = dp::RenderBucket;

  auto flushFn = [&](drape_ptr<TBucket> && bucket, ref_ptr<TBuffer> buffer)
  {
    if (buffer->GetIndexCount() == 0)
      return;

    ref_ptr<RenderGroup> oldGroup = make_ref(batches.front());
    drape_ptr<RenderGroup> newGroup = make_unique_dp<RenderGroup>(oldGroup->GetState(), oldGroup->GetTileKey());
    newGroup->m_shader = oldGroup->m_shader;
    newGroup->m_uniforms = oldGroup->m_uniforms;
    newGroup->m_generalUniforms = oldGroup->m_generalUniforms;
    newGroup->AddBucket(move(bucket));

    buffer->Preflush();
    buffer->Build(newGroup->m_shader);
    mergedBatches.push_back(move(newGroup));
  };

  auto allocateFn = [=](drape_ptr<TBucket> & bucket, ref_ptr<TBuffer> & buffer)
  {
    bucket = make_unique_dp<TBucket>(make_unique_dp<TBuffer>(kIndexBufferSize, kVertexBufferSize));
    buffer = bucket->GetBuffer();
  };

  auto copyVertecesFn = [](TBuffer::TBuffersMap::value_type const & vboNode,
                           vector<uint8_t> & rawDataBuffer,
                           ref_ptr<TBuffer> newBuffer)
  {
    dp::BindingInfo const & binding = vboNode.first;
    ref_ptr<dp::DataBufferBase> vbo = vboNode.second->GetBuffer();
    uint32_t vertexCount = vbo->GetCurrentSize();
    uint32_t bufferLength = vertexCount * vbo->GetElementSize();
    if (rawDataBuffer.size() < bufferLength)
      rawDataBuffer.resize(bufferLength);

    vbo->Bind();
    ReadBufferData(rawDataBuffer.data(), gl_const::GLArrayBuffer, bufferLength);
    GLFunctions::glUnmapBuffer(gl_const::GLArrayBuffer);

    newBuffer->UploadData(binding, rawDataBuffer.data(), vertexCount);
  };

  drape_ptr<TBucket> bucket;
  ref_ptr<TBuffer> newBuffer;
  allocateFn(bucket, newBuffer);

  vector<uint8_t> rawDataBuffer;

  for (drape_ptr<RenderGroup> const & group : batches)
  {
    for (drape_ptr<TBucket> const & b : group->m_renderBuckets)
    {
      ASSERT(b->m_overlay.empty(), ());
      ref_ptr<TBuffer> buffer = b->GetBuffer();
      uint32_t vertexCount = buffer->GetStartIndexValue();
      uint32_t indexCount = buffer->GetIndexCount();

      if (newBuffer->GetAvailableIndexCount() < vertexCount ||
          newBuffer->GetAvailableVertexCount() < indexCount)
      {
        flushFn(move(bucket), newBuffer);
        allocateFn(bucket, newBuffer);
      }

      uint32_t indexOffset = newBuffer->GetStartIndexValue();

      for (dp::VertexArrayBuffer::TBuffersMap::value_type const & vboNode : buffer->m_staticBuffers)
      {
        copyVertecesFn(vboNode, rawDataBuffer, newBuffer);
      }

      for (dp::VertexArrayBuffer::TBuffersMap::value_type const & vboNode : buffer->m_dynamicBuffers)
      {
        copyVertecesFn(vboNode, rawDataBuffer, newBuffer);
      }

      uint32_t indexByteCount = indexCount * dp::IndexStorage::SizeOfIndex();
      if (rawDataBuffer.size() < indexByteCount)
        rawDataBuffer.resize(indexByteCount);

      buffer->m_indexBuffer->GetBuffer()->Bind();
      ReadBufferData(rawDataBuffer.data(), gl_const::GLElementArrayBuffer, indexByteCount);
      GLFunctions::glUnmapBuffer(gl_const::GLElementArrayBuffer);

      if (dp::IndexStorage::IsSupported32bit())
        TransformIndeces<Supported32BitIndex>(rawDataBuffer.data(), indexCount, indexOffset);
      else
        TransformIndeces<NotSupported32BitIndex>(rawDataBuffer.data(), indexCount, indexOffset);

      newBuffer->UploadIndexes(rawDataBuffer.data(), indexCount);
    }
  }

  if (newBuffer->GetIndexCount() > 0)
  {
    flushFn(move(bucket), newBuffer);
    allocateFn(bucket, newBuffer);
  }
}

}
