#include "batcher.hpp"
#include "cpu_buffer.hpp"

#include "../base/assert.hpp"

#include "../std/utility.hpp"

namespace
{
  const uint32_t AllocateIndexCount = 3000;
  const uint32_t AllocateVertexCount = 3000;

  class IndexGenerator
  {
  public:
    IndexGenerator(uint16_t startIndex)
      : m_startIndex(startIndex)
      , m_counter(0)
    {
    }

  protected:
    uint16_t GetCounter()
    {
      return m_counter++;
    }

    const uint16_t m_startIndex;

  private:
    uint16_t m_counter;
  };

  class ListIndexGenerator : public IndexGenerator
  {
  public:
    ListIndexGenerator(uint16_t startIndex)
      : IndexGenerator(startIndex)
    {
    }

    uint16_t operator()()
    {
      return m_startIndex + GetCounter();
    }
  };

  class StripIndexGenerator : public IndexGenerator
  {
  public:
    StripIndexGenerator(uint16_t startIndex)
      : IndexGenerator(startIndex)
    {
    }

    uint16_t operator()()
    {
      uint16_t counter = GetCounter();
      return m_startIndex + counter - 2 * (counter / 3);
    }
  };

  class FanIndexGenerator : public IndexGenerator
  {
  public:
    FanIndexGenerator(uint16_t startIndex)
      : IndexGenerator(startIndex)
    {
    }

    uint16_t operator()()
    {
      uint16_t counter = GetCounter();
      if ((counter % 3) == 0)
        return m_startIndex;
      return m_startIndex + counter - 2 * (counter / 3);
    }
  };

  class InsertHelper
  {
  public:
    InsertHelper(RefPointer<AttributeProvider> params, RefPointer<VertexArrayBuffer> buffer)
      : m_params(params)
      , m_buffer(buffer)
    {
    }

    void GetUploadStripFanParams(uint16_t & resultVertexCount, uint16_t & resultIndexCount)
    {
      uint16_t vertexCount = m_params->GetVertexCount();
      uint16_t indexCount = VertexToIndexCount(vertexCount);
      resultVertexCount = vertexCount;
      resultIndexCount = VertexToIndexCount(vertexCount);

      uint16_t availableVertexCount = m_buffer->GetAvailableVertexCount();
      uint16_t availableIndexCount = m_buffer->GetAvailableIndexCount();

      if (vertexCount > availableVertexCount || indexCount > availableIndexCount)
      {
        uint32_t indexCountForAvailableVertexCount = VertexToIndexCount(availableVertexCount);
        if (indexCountForAvailableVertexCount <= availableIndexCount)
        {
          resultVertexCount = availableVertexCount;
          resultIndexCount = indexCountForAvailableVertexCount;
        }
        else
        {
          resultIndexCount = availableIndexCount - availableIndexCount % 3;
          resultVertexCount = IndexToVertexCount(resultIndexCount);
        }
      }
    }

    bool IsFullDataUploaded(uint16_t vertexCount)
    {
      return m_params->GetVertexCount() == vertexCount;
    }

  private:
    uint32_t VertexToIndexCount(uint32_t vertexCount)
    {
      return 3 * (vertexCount - 2);
    }

    uint32_t IndexToVertexCount(uint32_t indexCount)
    {
      return indexCount / 3 + 2;
    }

  private:
    RefPointer<AttributeProvider> m_params;
    RefPointer<VertexArrayBuffer> m_buffer;
  };
}

Batcher::Batcher()
{
}

Batcher::~Batcher()
{
  buckets_t::iterator it = m_buckets.begin();
  for (; it != m_buckets.end(); ++it)
    it->second.Destroy();
}

void Batcher::InsertTriangleList(const GLState & state, RefPointer<AttributeProvider> params)
{
  while (params->IsDataExists())
  {
    uint16_t vertexCount = params->GetVertexCount();

    RefPointer<VertexArrayBuffer> buffer = GetBuffer(state);
    ASSERT(!buffer->IsFilled(), ("Buffer must be filnalized on previous iteration"));

    uint16_t availableVertexCount = buffer->GetAvailableVertexCount();
    uint16_t availableIndexCount = buffer->GetAvailableIndexCount();
    vertexCount = min(vertexCount, availableVertexCount);
    vertexCount = min(vertexCount, availableIndexCount);

    ASSERT(vertexCount >= 3, ());

    vertexCount -= vertexCount % 3;

    vector<uint16_t> indexes;
    indexes.resize(vertexCount);
    generate(indexes.begin(), indexes.end(), ListIndexGenerator(buffer->GetStartIndexValue()));
    buffer->UploadIndexes(&indexes[0], vertexCount);

    /// upload data from params to GPU buffers
    for (size_t i = 0; i < params->GetStreamCount(); ++i)
    {
      RefPointer<GPUBuffer> streamBuffer = buffer->GetBuffer(params->GetBindingInfo(i));
      streamBuffer->UploadData(params->GetRawPointer(i), vertexCount);
    }

    params->Advance(vertexCount);
    if (buffer->IsFilled())
    {
      buffer = RefPointer<VertexArrayBuffer>();
      FinalizeBuffer(state);
    }
  }
}

void Batcher::InsertTriangleStrip(const GLState & state, RefPointer<AttributeProvider> params)
{
  while (params->IsDataExists())
  {
    RefPointer<VertexArrayBuffer> buffer = GetBuffer(state);
    ASSERT(!buffer->IsFilled(), ("Buffer must be filnalized on previous iteration"));

    InsertHelper helper(params, buffer);

    uint16_t vertexCount, indexCount;
    helper.GetUploadStripFanParams(vertexCount, indexCount);

    // generate indexes
    vector<uint16_t> indexes;
    indexes.resize(indexCount);
    generate(indexes.begin(), indexes.end(), StripIndexGenerator(buffer->GetStartIndexValue()));
    buffer->UploadIndexes(&indexes[0], indexCount);

    for (size_t i = 0; i < params->GetStreamCount(); ++i)
    {
      RefPointer<GPUBuffer> streamBuffer = buffer->GetBuffer(params->GetBindingInfo(i));
      streamBuffer->UploadData(params->GetRawPointer(i), vertexCount);
    }

    if (helper.IsFullDataUploaded(vertexCount))
      params->Advance(vertexCount);
    else
      // uploadVertexCount - 2 for copy last 2 vertex into next VAO as this is TriangleStrip
      params->Advance(vertexCount - 2);

    if (buffer->IsFilled())
    {
      buffer = RefPointer<VertexArrayBuffer>();
      FinalizeBuffer(state);
    }
  }
}


/*
 * What happens here
 *
 * We try to pack TriangleFan on GPU indexed like triangle list.
 * If we have enough memory in VertexArrayBuffer to store all data from params, we just copy it
 *
 * If we have not enough memory we broke data on parts.
 * On first iteration we create CPUBuffer for each separate atribute
 * in params and copy to it first vertex of fan. This vertex will be need
 * when we will upload second part of data.
 *
 * Than we copy vertex data on GPU as much as we can and move params cursor on
 * "uploaded vertex count" - 1. This last vertex will be used for uploading next part of data
 *
 * On second iteration we need upload first vertex of fan that stored in cpuBuffers and than upload
 * second part of data. But to avoid 2 separate call of glBufferSubData we at first do a copy of
 * data from params to cpuBuffer and than copy continuous block of memory from cpuBuffer
 */
void Batcher::InsertTriangleFan(const GLState & state, RefPointer<AttributeProvider> params)
{
  vector<CPUBuffer> cpuBuffers;
  while (params->IsDataExists())
  {
    RefPointer<VertexArrayBuffer> buffer = GetBuffer(state);
    ASSERT(!buffer->IsFilled(), ("Buffer must be filnalized on previous iteration"));

    InsertHelper helper(params, buffer);

    uint16_t vertexCount, indexCount;
    helper.GetUploadStripFanParams(vertexCount, indexCount);

    // generate indexes
    vector<uint16_t> indexes;
    indexes.resize(indexCount);
    generate(indexes.begin(), indexes.end(), FanIndexGenerator(buffer->GetStartIndexValue()));
    buffer->UploadIndexes(&indexes[0], indexCount);

    if (!cpuBuffers.empty())
    {
      // if cpuBuffers not empty than on previous interation we not move data on gpu
      // and in cpuBuffers stored first vertex of fan.
      // To avoid two separate call of glBufferSubData
      // (for first vertex and for next part of data)
      // we at first copy next part of data into
      // cpuBuffers, and than copy it from cpuBuffers to GPU
      for (size_t i = 0; i < params->GetStreamCount(); ++i)
      {
        CPUBuffer & cpuBuffer = cpuBuffers[i];
        ASSERT(cpuBuffer.GetCurrentElementNumber() == 1, ());
        cpuBuffer.UploadData(params->GetRawPointer(i), vertexCount);

        RefPointer<GPUBuffer> streamBuffer = buffer->GetBuffer(params->GetBindingInfo(i));
        // now in cpuBuffer we have correct "fan" created from second part of data
        // first vertex of cpuBuffer if the first vertex of params, second vertex is
        // the last vertex of previous uploaded data. We copy this data on GPU.
        streamBuffer->UploadData(cpuBuffer.Data(), vertexCount + 1);

        // Move cpu buffer cursor on second element of buffer.
        // On next iteration first vertex of fan will be also available
        cpuBuffer.Seek(1);
      }

      if (helper.IsFullDataUploaded(vertexCount))
      {
        // this means that we move all data on gpu
        params->Advance(vertexCount);
      }
      else
      {
        // not all data was moved on gpu and last vertex of fan
        // will need on second iteration
        params->Advance(vertexCount - 1);
      }
    }
    else // if m_cpuBuffer empty than it's first iteration
    {
      if (helper.IsFullDataUploaded(vertexCount))
      {
        // We can upload all input data as one peace. For upload we need only one iteration
        for (size_t i = 0; i < params->GetStreamCount(); ++i)
        {
          RefPointer<GPUBuffer> streamBuffer = buffer->GetBuffer(params->GetBindingInfo(i));
          streamBuffer->UploadData(params->GetRawPointer(i), vertexCount);
        }
        params->Advance(vertexCount);
      }
      else
      {
        // for each stream we must create CPU buffer.
        // Copy first vertex of fan into cpuBuffer for next iterations
        // Than move first part of data on GPU
        cpuBuffers.reserve(params->GetStreamCount());
        for (size_t i = 0; i < params->GetStreamCount(); ++i)
        {
          const BindingInfo & binding = params->GetBindingInfo(i);
          const void * rawDataPointer = params->GetRawPointer(i);
          RefPointer<GPUBuffer> streamBuffer = buffer->GetBuffer(binding);
          streamBuffer->UploadData(rawDataPointer, vertexCount);

          cpuBuffers.push_back(CPUBuffer(binding.GetElementSize(), vertexCount));
          CPUBuffer & cpuBuffer = cpuBuffers.back();
          cpuBuffer.UploadData(rawDataPointer, 1);
        }

        // advance on uploadVertexCount - 1 to copy last vertex also into next VAO with
        // first vertex of data from CPUBuffers
        params->Advance(vertexCount - 1);
      }
    }

    if (buffer->IsFilled())
    {
      buffer = RefPointer<VertexArrayBuffer>();
      FinalizeBuffer(state);
    }
  }
}

void Batcher::StartSession(const flush_fn & flusher)
{
  m_flushInterface = flusher;
}

void Batcher::EndSession()
{
  Flush();
  m_flushInterface = flush_fn();
}

RefPointer<VertexArrayBuffer> Batcher::GetBuffer(const GLState & state)
{
  buckets_t::iterator it = m_buckets.find(state);
  if (it != m_buckets.end())
    return it->second.GetRefPointer();

  MasterPointer<VertexArrayBuffer> buffer(new VertexArrayBuffer(AllocateIndexCount, AllocateVertexCount));
  m_buckets.insert(make_pair(state, buffer));
  return buffer.GetRefPointer();
}

void Batcher::FinalizeBuffer(const GLState & state)
{
  ASSERT(m_buckets.find(state) != m_buckets.end(), ("Have no bucket for finalize with given state"));
  m_flushInterface(state, m_buckets[state].Move());
  m_buckets.erase(state);
}

void Batcher::Flush()
{
  for (buckets_t::iterator it = m_buckets.begin(); it != m_buckets.end(); ++it)
    m_flushInterface(it->first, it->second.Move());

  m_buckets.clear();
}
