#include "drape/batcher_helpers.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/cpu_buffer.hpp"
#include "drape/index_storage.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include <algorithm>

namespace dp
{
namespace
{
template <typename T>
T CyclicClamp(T const x, T const xmin, T const xmax)
{
  if (x > xmax)
    return xmin;
  if (x < xmin)
    return xmax;
  return x;
}

bool IsEnoughMemory(uint32_t avVertex, uint32_t existVertex, uint32_t avIndex, uint32_t existIndex)
{
  return avVertex >= existVertex && avIndex >= existIndex;
}

template <typename TGenerator>
void GenerateIndices(void * indexStorage, uint32_t count, uint32_t startIndex)
{
  GenerateIndices<TGenerator>(indexStorage, count, TGenerator(startIndex));
}

template <typename TGenerator>
void GenerateIndices(void * indexStorage, uint32_t count, TGenerator const & generator)
{
  if (dp::IndexStorage::IsSupported32bit())
  {
    auto pIndexStorage = static_cast<uint32_t *>(indexStorage);
    std::generate(pIndexStorage, pIndexStorage + count, generator);
  }
  else
  {
    auto pIndexStorage = static_cast<uint16_t *>(indexStorage);
    std::generate(pIndexStorage, pIndexStorage + count, generator);
  }
}

class IndexGenerator
{
public:
  explicit IndexGenerator(uint32_t startIndex) : m_startIndex(startIndex), m_counter(0), m_minStripCounter(0) {}

protected:
  uint32_t GetCounter() { return m_counter++; }

  void ResetCounter()
  {
    m_counter = 0;
    m_minStripCounter = 0;
  }

  uint32_t const m_startIndex;

  int16_t GetCWNormalizer()
  {
    int16_t tmp = m_minStripCounter;
    m_minStripCounter = static_cast<uint8_t>(CyclicClamp(m_minStripCounter + 1, 0, 5));
    switch (tmp)
    {
    case 4: return 1;
    case 5: return -1;
    default: return 0;
    }
  }

private:
  uint32_t m_counter;
  uint8_t m_minStripCounter;
};

class ListIndexGenerator : public IndexGenerator
{
public:
  explicit ListIndexGenerator(uint32_t startIndex) : IndexGenerator(startIndex) {}
  uint32_t operator()() { return m_startIndex + GetCounter(); }
};

class StripIndexGenerator : public IndexGenerator
{
public:
  explicit StripIndexGenerator(uint32_t startIndex) : IndexGenerator(startIndex) {}

  uint32_t operator()()
  {
    uint32_t const counter = GetCounter();
    return m_startIndex + counter - 2 * (counter / 3) + GetCWNormalizer();
  }
};

class LineStripIndexGenerator : public IndexGenerator
{
public:
  explicit LineStripIndexGenerator(uint32_t startIndex) : IndexGenerator(startIndex) {}

  uint32_t operator()()
  {
    uint32_t const counter = GetCounter();
    uint32_t const result = m_startIndex + m_counter;
    if (counter % 2 == 0)
      m_counter++;

    return result;
  }

private:
  uint32_t m_counter = 0;
};

class LineRawIndexGenerator : public IndexGenerator
{
public:
  LineRawIndexGenerator(uint32_t startIndex, std::vector<int> const & indices)
    : IndexGenerator(startIndex)
    , m_indices(indices)
  {}

  uint32_t operator()()
  {
    uint32_t const counter = GetCounter();
    ASSERT_LESS(counter, m_indices.size(), ());
    return static_cast<uint32_t>(m_startIndex + m_indices[counter]);
  }

private:
  std::vector<int> const & m_indices;
};

class FanIndexGenerator : public IndexGenerator
{
public:
  explicit FanIndexGenerator(uint32_t startIndex) : IndexGenerator(startIndex) {}

  uint32_t operator()()
  {
    uint32_t const counter = GetCounter();
    if ((counter % 3) == 0)
      return m_startIndex;
    return m_startIndex + counter - 2 * (counter / 3);
  }
};

class ListOfStripGenerator : public IndexGenerator
{
public:
  ListOfStripGenerator(uint32_t startIndex, uint32_t vertexStride, uint32_t indexPerStrip)
    : IndexGenerator(startIndex)
    , m_vertexStride(vertexStride)
    , m_indexPerStrip(indexPerStrip)
    , m_base(0)
  {}

  uint32_t operator()()
  {
    uint32_t const counter = GetCounter();
    uint32_t const result = m_startIndex + m_base + counter - 2 * (counter / 3) + GetCWNormalizer();
    if (counter + 1 == m_indexPerStrip)
    {
      m_base += m_vertexStride;
      ResetCounter();
    }

    return result;
  }

private:
  uint32_t m_vertexStride;
  uint32_t m_indexPerStrip;
  uint32_t m_base;
};
}  // namespace

UniversalBatch::UniversalBatch(BatchCallbacks & callbacks, uint8_t minVerticesCount, uint8_t minIndicesCount)
  : m_callbacks(callbacks)
  , m_canDivideStreams(true)
  , m_minVerticesCount(minVerticesCount)
  , m_minIndicesCount(minIndicesCount)
{}

void UniversalBatch::SetCanDivideStreams(bool canDivide)
{
  m_canDivideStreams = canDivide;
}

bool UniversalBatch::CanDivideStreams() const
{
  return m_canDivideStreams;
}

void UniversalBatch::SetVertexStride(uint8_t vertexStride)
{
  m_vertexStride = vertexStride;
}

void UniversalBatch::FlushData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams,
                               uint32_t vertexCount) const
{
  for (uint8_t i = 0; i < streams->GetStreamCount(); ++i)
    FlushData(context, streams->GetBindingInfo(i), streams->GetRawPointer(i), vertexCount);
}

void UniversalBatch::FlushData(ref_ptr<GraphicsContext> context, BindingInfo const & info, void const * data,
                               uint32_t elementCount) const
{
  m_callbacks.FlushData(context, info, data, elementCount);
}

void * UniversalBatch::GetIndexStorage(uint32_t indexCount, uint32_t & startIndex)
{
  return m_callbacks.GetIndexStorage(indexCount, startIndex);
}

void UniversalBatch::SubmitIndices(ref_ptr<GraphicsContext> context)
{
  m_callbacks.SubmitIndices(context);
}

uint32_t UniversalBatch::GetAvailableVertexCount() const
{
  return m_callbacks.GetAvailableVertexCount();
}

uint32_t UniversalBatch::GetAvailableIndexCount() const
{
  return m_callbacks.GetAvailableIndexCount();
}

void UniversalBatch::ChangeBuffer(ref_ptr<GraphicsContext> context) const
{
  return m_callbacks.ChangeBuffer(context);
}

uint8_t UniversalBatch::GetVertexStride() const
{
  return m_vertexStride;
}

bool UniversalBatch::IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const
{
  return availableVerticesCount < m_minVerticesCount || availableIndicesCount < m_minIndicesCount;
}

TriangleListBatch::TriangleListBatch(BatchCallbacks & callbacks)
  : TBase(callbacks, 3 /* minVerticesCount */, 3 /* minIndicesCount */)
{}

void TriangleListBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t avVertex = GetAvailableVertexCount();
    uint32_t avIndex = GetAvailableIndexCount();
    uint32_t vertexCount = streams->GetVertexCount();

    if (CanDivideStreams())
    {
      vertexCount = std::min(vertexCount, avVertex);
      vertexCount = std::min(vertexCount, avIndex);
      ASSERT(vertexCount >= 3, ());
      vertexCount -= vertexCount % 3;
    }
    else if (!IsEnoughMemory(avVertex, vertexCount, avIndex, vertexCount))
    {
      ChangeBuffer(context);
      avVertex = GetAvailableVertexCount();
      avIndex = GetAvailableIndexCount();
      ASSERT(IsEnoughMemory(avVertex, vertexCount, avIndex, vertexCount), ());
      ASSERT(vertexCount % 3 == 0, ());
    }

    uint32_t startIndex = 0;
    void * indicesStorage = GetIndexStorage(vertexCount, startIndex);
    GenerateIndices<ListIndexGenerator>(indicesStorage, vertexCount, startIndex);
    SubmitIndices(context);

    FlushData(context, streams, vertexCount);
    streams->Advance(vertexCount);
  }
}

LineStripBatch::LineStripBatch(BatchCallbacks & callbacks)
  : TBase(callbacks, 2 /* minVerticesCount */, 2 /* minIndicesCount */)
{}

void LineStripBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t avVertex = GetAvailableVertexCount();
    uint32_t avIndex = GetAvailableIndexCount();
    uint32_t vertexCount = streams->GetVertexCount();
    ASSERT_GREATER_OR_EQUAL(vertexCount, 2, ());
    uint32_t indexCount = (vertexCount - 1) * 2;

    if (!IsEnoughMemory(avVertex, vertexCount, avIndex, indexCount))
    {
      ChangeBuffer(context);
      avVertex = GetAvailableVertexCount();
      avIndex = GetAvailableIndexCount();
      ASSERT(IsEnoughMemory(avVertex, vertexCount, avIndex, indexCount), ());
    }

    uint32_t startIndex = 0;
    void * indicesStorage = GetIndexStorage(indexCount, startIndex);
    GenerateIndices<LineStripIndexGenerator>(indicesStorage, indexCount, startIndex);
    SubmitIndices(context);

    FlushData(context, streams, vertexCount);
    streams->Advance(vertexCount);
  }
}

LineRawBatch::LineRawBatch(BatchCallbacks & callbacks, std::vector<int> const & indices)
  : TBase(callbacks, 2 /* minVerticesCount */, 2 /* minIndicesCount */)
  , m_indices(indices)
{}

void LineRawBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t avVertex = GetAvailableVertexCount();
    uint32_t avIndex = GetAvailableIndexCount();
    uint32_t vertexCount = streams->GetVertexCount();
    CHECK_GREATER_OR_EQUAL(vertexCount, 2, (vertexCount));
    auto const indexCount = static_cast<uint32_t>(m_indices.size());

    if (!IsEnoughMemory(avVertex, vertexCount, avIndex, indexCount))
    {
      ChangeBuffer(context);
      avVertex = GetAvailableVertexCount();
      avIndex = GetAvailableIndexCount();
      CHECK(IsEnoughMemory(avVertex, vertexCount, avIndex, indexCount), (avVertex, vertexCount, avIndex, indexCount));
    }

    uint32_t startIndex = 0;
    void * indicesStorage = GetIndexStorage(indexCount, startIndex);
    LineRawIndexGenerator generator(startIndex, m_indices);
    GenerateIndices(indicesStorage, indexCount, generator);
    SubmitIndices(context);

    FlushData(context, streams, vertexCount);
    streams->Advance(vertexCount);
  }
}

FanStripHelper::FanStripHelper(BatchCallbacks & callbacks)
  : TBase(callbacks, 3 /* minVerticesCount */, 3 /* minIndicesCount */)
  , m_isFullUploaded(false)
{}

uint32_t FanStripHelper::BatchIndexes(ref_ptr<GraphicsContext> context, uint32_t vertexCount)
{
  uint32_t avVertex = GetAvailableVertexCount();
  uint32_t avIndex = GetAvailableIndexCount();

  uint32_t batchVertexCount = 0;
  uint32_t batchIndexCount = 0;
  CalcBatchPortion(vertexCount, avVertex, avIndex, batchVertexCount, batchIndexCount);

  if (!IsFullUploaded() && !CanDivideStreams())
  {
    ChangeBuffer(context);
    avVertex = GetAvailableVertexCount();
    avIndex = GetAvailableIndexCount();
    CalcBatchPortion(vertexCount, avVertex, avIndex, batchVertexCount, batchIndexCount);
    ASSERT(IsFullUploaded(), ());
  }

  uint32_t startIndex = 0;
  void * pIndexStorage = GetIndexStorage(batchIndexCount, startIndex);
  GenerateIndexes(pIndexStorage, batchIndexCount, startIndex);
  SubmitIndices(context);

  return batchVertexCount;
}

void FanStripHelper::CalcBatchPortion(uint32_t vertexCount, uint32_t avVertex, uint32_t avIndex,
                                      uint32_t & batchVertexCount, uint32_t & batchIndexCount)
{
  uint32_t const indexCount = VtoICount(vertexCount);
  batchVertexCount = vertexCount;
  batchIndexCount = indexCount;
  m_isFullUploaded = true;

  if (vertexCount > avVertex || indexCount > avIndex)
  {
    uint32_t alignedAvVertex = AlignVCount(avVertex);
    uint32_t alignedAvIndex = AlignICount(avIndex);
    uint32_t indexCountForAvailableVertexCount = VtoICount(alignedAvVertex);
    if (indexCountForAvailableVertexCount <= alignedAvIndex)
    {
      batchVertexCount = alignedAvVertex;
      batchIndexCount = indexCountForAvailableVertexCount;
    }
    else
    {
      batchIndexCount = alignedAvIndex;
      batchVertexCount = ItoVCount(batchIndexCount);
    }
    m_isFullUploaded = false;
  }
}

bool FanStripHelper::IsFullUploaded() const
{
  return m_isFullUploaded;
}

uint32_t FanStripHelper::VtoICount(uint32_t vCount) const
{
  return 3 * (vCount - 2);
}

uint32_t FanStripHelper::ItoVCount(uint32_t iCount) const
{
  return iCount / 3 + 2;
}

uint32_t FanStripHelper::AlignVCount(uint32_t vCount) const
{
  return vCount;
}

uint32_t FanStripHelper::AlignICount(uint32_t iCount) const
{
  return iCount - iCount % 3;
}

TriangleStripBatch::TriangleStripBatch(BatchCallbacks & callbacks) : TBase(callbacks) {}

void TriangleStripBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t const batchVertexCount = BatchIndexes(context, streams->GetVertexCount());
    FlushData(context, streams, batchVertexCount);

    uint32_t const advanceCount = IsFullUploaded() ? batchVertexCount : (batchVertexCount - 2);
    streams->Advance(advanceCount);
  }
}

void TriangleStripBatch::GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const
{
  GenerateIndices<StripIndexGenerator>(indexStorage, count, startIndex);
}

TriangleFanBatch::TriangleFanBatch(BatchCallbacks & callbacks) : TBase(callbacks) {}

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
void TriangleFanBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  std::vector<CPUBuffer> cpuBuffers;
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t vertexCount = streams->GetVertexCount();
    uint32_t batchVertexCount = BatchIndexes(context, vertexCount);

    if (!cpuBuffers.empty())
    {
      // if cpuBuffers not empty than on previous iteration we not move data on gpu
      // and in cpuBuffers stored first vertex of fan.
      // To avoid two separate call of glBufferSubData
      // (for first vertex and for next part of data)
      // we at first copy next part of data into
      // cpuBuffers, and than copy it from cpuBuffers to GPU
      for (size_t i = 0; i < streams->GetStreamCount(); ++i)
      {
        CPUBuffer & cpuBuffer = cpuBuffers[i];
        ASSERT(cpuBuffer.GetCurrentElementNumber() == 1, ());
        cpuBuffer.UploadData(streams->GetRawPointer(i), batchVertexCount);

        // now in cpuBuffer we have correct "fan" created from second part of data
        // first vertex of cpuBuffer if the first vertex of params, second vertex is
        // the last vertex of previous uploaded data. We copy this data on GPU.
        FlushData(context, streams->GetBindingInfo(i), cpuBuffer.Data(), batchVertexCount + 1);
      }

      uint32_t advanceCount = batchVertexCount;
      if (!IsFullUploaded())
      {
        // not all data was moved on gpu and last vertex of fan
        // will need on second iteration
        advanceCount -= 1;
      }

      streams->Advance(advanceCount);
    }
    else  // if m_cpuBuffer empty than it's first iteration
      if (IsFullUploaded())
      {
        // We can upload all input data as one peace. For upload we need only one iteration
        FlushData(context, streams, batchVertexCount);
        streams->Advance(batchVertexCount);
      }
      else
      {
        // for each stream we must create CPU buffer.
        // Copy first vertex of fan into cpuBuffer for next iterations
        // Than move first part of data on GPU
        cpuBuffers.reserve(streams->GetStreamCount());
        for (size_t i = 0; i < streams->GetStreamCount(); ++i)
        {
          BindingInfo const & binding = streams->GetBindingInfo(i);
          void const * rawDataPointer = streams->GetRawPointer(i);
          FlushData(context, binding, rawDataPointer, batchVertexCount);

          // "(vertexCount + 1) - batchVertexCount" we allocate CPUBuffer on all remaining data
          // + first vertex of fan, that must be duplicate in the next buffer
          // + last vertex of currently uploaded data.
          cpuBuffers.emplace_back(binding.GetElementSize(), (vertexCount + 2) - batchVertexCount);
          CPUBuffer & cpuBuffer = cpuBuffers.back();
          cpuBuffer.UploadData(rawDataPointer, 1);

          // Move cpu buffer cursor on second element of buffer.
          // On next iteration first vertex of fan will be also available
          cpuBuffer.Seek(1);
        }

        // advance on uploadVertexCount - 1 to copy last vertex also into next VAO with
        // first vertex of data from CPUBuffers
        streams->Advance(batchVertexCount - 1);
      }
  }
}

void TriangleFanBatch::GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const
{
  GenerateIndices<FanIndexGenerator>(indexStorage, count, startIndex);
}

TriangleListOfStripBatch::TriangleListOfStripBatch(BatchCallbacks & callbacks) : TBase(callbacks) {}

void TriangleListOfStripBatch::BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    if (IsBufferFilled(GetAvailableVertexCount(), GetAvailableIndexCount()))
      ChangeBuffer(context);

    uint32_t const batchVertexCount = BatchIndexes(context, streams->GetVertexCount());
    FlushData(context, streams, batchVertexCount);
    streams->Advance(batchVertexCount);
  }
}

bool TriangleListOfStripBatch::IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());

  uint32_t const indicesPerStride = TBase::VtoICount(vertexStride);
  return availableVerticesCount < vertexStride || availableIndicesCount < indicesPerStride;
}

uint32_t TriangleListOfStripBatch::VtoICount(uint32_t vCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());
  ASSERT_EQUAL(vCount % vertexStride, 0, ());

  uint32_t const striptCount = vCount / vertexStride;
  return striptCount * TBase::VtoICount(vertexStride);
}

uint32_t TriangleListOfStripBatch::ItoVCount(uint32_t iCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());
  ASSERT_EQUAL(iCount % 3, 0, ());

  return vertexStride * iCount / TBase::VtoICount(vertexStride);
}

uint32_t TriangleListOfStripBatch::AlignVCount(uint32_t vCount) const
{
  return vCount - vCount % GetVertexStride();
}

uint32_t TriangleListOfStripBatch::AlignICount(uint32_t iCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());

  uint32_t const indicesPerStride = TBase::VtoICount(vertexStride);
  return iCount - iCount % indicesPerStride;
}

void TriangleListOfStripBatch::GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const
{
  uint8_t const vertexStride = GetVertexStride();
  GenerateIndices(indexStorage, count, ListOfStripGenerator(startIndex, vertexStride, VtoICount(vertexStride)));
}
}  // namespace dp
