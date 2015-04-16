#include "drape/batcher_helpers.hpp"
#include "drape/attribute_provider.hpp"
#include "drape/cpu_buffer.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"

#include "std/algorithm.hpp"

namespace dp
{

namespace
{

bool IsEnoughMemory(uint16_t avVertex, uint16_t existVertex, uint16_t avIndex, uint16_t existIndex)
{
  return avVertex >= existVertex && avIndex >= existIndex;
}

class IndexGenerator
{
public:
  IndexGenerator(uint16_t startIndex)
    : m_startIndex(startIndex)
    , m_counter(0)
    , m_minStriptCounter(0) {}

protected:
  uint16_t GetCounter() { return m_counter++; }
  void ResetCounter()
  {
    m_counter = 0;
    m_minStriptCounter = 0;
  }

  uint16_t const m_startIndex;

  int16_t GetCWNormalizer()
  {
    int16_t tmp = m_minStriptCounter;
    m_minStriptCounter = my::cyclicClamp(m_minStriptCounter + 1, 0, 5);
    switch (tmp)
    {
      case 4: return 1;
      case 5: return -1;
      default: return 0;
    }
  }

private:
  uint16_t m_counter;
  uint8_t m_minStriptCounter;
};

class ListIndexGenerator : public IndexGenerator
{
public:
  ListIndexGenerator(uint16_t startIndex) : IndexGenerator(startIndex) {}
  uint16_t operator()() { return m_startIndex + GetCounter(); }
};

class StripIndexGenerator : public IndexGenerator
{
public:
  StripIndexGenerator(uint16_t startIndex)
    : IndexGenerator(startIndex)
    , m_minStriptCounter(0) {}
  uint16_t operator()()
  {
    uint16_t const counter = GetCounter();
    return m_startIndex + counter - 2 * (counter / 3) + GetCWNormalizer();
  }

private:
  uint16_t m_minStriptCounter;
};

class FanIndexGenerator : public IndexGenerator
{
public:
  FanIndexGenerator(uint16_t startIndex) : IndexGenerator(startIndex) {}
  uint16_t operator()()
  {
    uint16_t const counter = GetCounter();
    if ((counter % 3) == 0)
      return m_startIndex;
    return m_startIndex + counter - 2 * (counter / 3);
  }
};

class ListOfStriptGenerator : public IndexGenerator
{
public:
  ListOfStriptGenerator(uint16_t startIndex, uint16_t vertexStride, uint16_t indexPerStrip)
    : IndexGenerator(startIndex)
    , m_vertexStride(vertexStride)
    , m_indexPerStrip(indexPerStrip)
    , m_base(0) {}

  uint16_t operator()()
  {
    uint16_t const counter = GetCounter();
    uint16_t const result = m_startIndex + m_base + counter - 2 * (counter / 3) + GetCWNormalizer();
    if (counter + 1 == m_indexPerStrip)
    {
      m_base += m_vertexStride;
      ResetCounter();
    }

    return result;
  }

private:
  uint16_t m_vertexStride;
  uint16_t m_indexPerStrip;
  uint16_t m_base;
};

} // namespace

TriangleBatch::TriangleBatch(BatchCallbacks const & callbacks)
  : m_callbacks(callbacks)
  , m_canDevideStreams(true)
{
}

void TriangleBatch::SetIsCanDevideStreams(bool canDevide)
{
  m_canDevideStreams = canDevide;
}

bool TriangleBatch::IsCanDevideStreams() const
{
  return m_canDevideStreams;
}

void TriangleBatch::SetVertexStride(uint8_t vertexStride)
{
  m_vertexStride = vertexStride;
}

void TriangleBatch::FlushData(ref_ptr<AttributeProvider> streams, uint16_t vertexVount) const
{
  for (uint8_t i = 0; i < streams->GetStreamCount(); ++i)
    FlushData(streams->GetBindingInfo(i), streams->GetRawPointer(i), vertexVount);
}

void TriangleBatch::FlushData(BindingInfo const & info, void const * data, uint16_t elementCount) const
{
  ASSERT(m_callbacks.m_flushVertex != NULL, ());
  return m_callbacks.m_flushVertex(info, data, elementCount);
}

uint16_t * TriangleBatch::GetIndexStorage(uint16_t indexCount, uint16_t & startIndex)
{
  ASSERT(m_callbacks.m_getIndexStorage, ());
  return m_callbacks.m_getIndexStorage(indexCount, startIndex);
}

void TriangleBatch::SubmitIndex()
{
  ASSERT(m_callbacks.m_submitIndex, ());
  m_callbacks.m_submitIndex();
}

uint16_t TriangleBatch::GetAvailableVertexCount() const
{
  ASSERT(m_callbacks.m_getAvailableVertex!= NULL, ());
  return m_callbacks.m_getAvailableVertex();
}

uint16_t TriangleBatch::GetAvailableIndexCount() const
{
  ASSERT(m_callbacks.m_getAvailableIndex != NULL, ());
  return m_callbacks.m_getAvailableIndex();
}

void TriangleBatch::ChangeBuffer(bool checkFilled) const
{
  ASSERT(m_callbacks.m_changeBuffer != NULL, ());
  m_callbacks.m_changeBuffer(checkFilled);
}

uint8_t TriangleBatch::GetVertexStride() const
{
  return m_vertexStride;
}

TriangleListBatch::TriangleListBatch(BatchCallbacks const & callbacks) : TBase(callbacks) {}

void TriangleListBatch::BatchData(ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    ChangeBuffer(true);
    uint16_t avVertex = GetAvailableVertexCount();
    uint16_t avIndex  = GetAvailableIndexCount();
    uint16_t vertexCount = streams->GetVertexCount();

    if (IsCanDevideStreams())
    {
      vertexCount = min(vertexCount, avVertex);
      vertexCount = min(vertexCount, avIndex);
      ASSERT(vertexCount >= 3, ());
      vertexCount -= vertexCount % 3;
    }
    else if (!IsEnoughMemory(avVertex, vertexCount, avIndex, vertexCount))
    {
      ChangeBuffer(false);
      avVertex = GetAvailableVertexCount();
      avIndex  = GetAvailableIndexCount();
      ASSERT(IsEnoughMemory(avVertex, vertexCount, avIndex, vertexCount), ());
      ASSERT(vertexCount % 3 == 0, ());
    }

    uint16_t startIndex = 0;
    uint16_t * pIndexStorage = GetIndexStorage(vertexCount, startIndex);
    generate(pIndexStorage, pIndexStorage + vertexCount, ListIndexGenerator(startIndex));
    SubmitIndex();

    FlushData(streams, vertexCount);
    streams->Advance(vertexCount);
  }
}


FanStripHelper::FanStripHelper(BatchCallbacks const & callbacks)
  : TBase(callbacks)
  , m_isFullUploaded(false)
{
}

uint16_t FanStripHelper::BatchIndexes(uint16_t vertexCount)
{
  uint16_t avVertex = GetAvailableVertexCount();
  uint16_t avIndex  = GetAvailableIndexCount();

  uint16_t batchVertexCount = 0;
  uint16_t batchIndexCount = 0;
  CalcBatchPortion(vertexCount, avVertex, avIndex, batchVertexCount, batchIndexCount);

  if (!IsFullUploaded() && !IsCanDevideStreams())
  {
    ChangeBuffer(false);
    avVertex = GetAvailableVertexCount();
    avIndex  = GetAvailableIndexCount();
    CalcBatchPortion(vertexCount, avVertex, avIndex, batchVertexCount, batchIndexCount);
    ASSERT(IsFullUploaded(), ());
  }

  uint16_t startIndex = 0;
  uint16_t * pIndexStorage = GetIndexStorage(batchIndexCount, startIndex);
  GenerateIndexes(pIndexStorage, batchIndexCount, startIndex);
  SubmitIndex();

  return batchVertexCount;
}

void FanStripHelper::CalcBatchPortion(uint16_t vertexCount, uint16_t avVertex, uint16_t avIndex,
                                      uint16_t & batchVertexCount, uint16_t & batchIndexCount)
{
  uint16_t const indexCount = VtoICount(vertexCount);
  batchVertexCount = vertexCount;
  batchIndexCount = indexCount;
  m_isFullUploaded = true;

  if (vertexCount > avVertex || indexCount > avIndex)
  {
    uint16_t alignedAvVertex = AlignVCount(avVertex);
    uint16_t alignedAvIndex = AlignICount(avIndex);
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

uint16_t FanStripHelper::VtoICount(uint16_t vCount) const
{
  return 3 * (vCount - 2);
}

uint16_t FanStripHelper::ItoVCount(uint16_t iCount) const
{
  return iCount / 3 + 2;
}

uint16_t FanStripHelper::AlignVCount(uint16_t vCount) const
{
  return vCount;
}

uint16_t FanStripHelper::AlignICount(uint16_t iCount) const
{
  return iCount - iCount % 3;
}

TriangleStripBatch::TriangleStripBatch(BatchCallbacks const & callbacks)
 : TBase(callbacks)
{
}

void TriangleStripBatch::BatchData(ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    ChangeBuffer(true);
    uint16_t const batchVertexCount = BatchIndexes(streams->GetVertexCount());
    FlushData(streams, batchVertexCount);

    uint16_t const advanceCount = IsFullUploaded() ? batchVertexCount : (batchVertexCount - 2);
    streams->Advance(advanceCount);

  }
}

void TriangleStripBatch::GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const
{
  generate(indexStorage, indexStorage + count, StripIndexGenerator(startIndex));
}

TriangleFanBatch::TriangleFanBatch(BatchCallbacks const & callbacks) : TBase(callbacks) {}


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
void TriangleFanBatch::BatchData(ref_ptr<AttributeProvider> streams)
{
  vector<CPUBuffer> cpuBuffers;
  while (streams->IsDataExists())
  {
    ChangeBuffer(true);
    uint16_t vertexCount = streams->GetVertexCount();
    uint16_t batchVertexCount = BatchIndexes(vertexCount);

    if (!cpuBuffers.empty())
    {
      // if cpuBuffers not empty than on previous interation we not move data on gpu
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
        FlushData(streams->GetBindingInfo(i), cpuBuffer.Data(), batchVertexCount + 1);

        // Move cpu buffer cursor on second element of buffer.
        // On next iteration first vertex of fan will be also available
        cpuBuffer.Seek(1);
      }

      uint16_t advanceCount = batchVertexCount;
      if (!IsFullUploaded())
      {
        // not all data was moved on gpu and last vertex of fan
        // will need on second iteration
        advanceCount -= 1;
      }

      streams->Advance(advanceCount);
    }
    else // if m_cpuBuffer empty than it's first iteration
    {
      if (IsFullUploaded())
      {
        // We can upload all input data as one peace. For upload we need only one iteration
        FlushData(streams, batchVertexCount);
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
          const BindingInfo & binding = streams->GetBindingInfo(i);
          const void * rawDataPointer = streams->GetRawPointer(i);
          FlushData(binding, rawDataPointer, batchVertexCount);

          /// "(vertexCount + 1) - batchVertexCount" we allocate CPUBuffer on all remaining data
          /// + first vertex of fan, that must be duplicate in nex buffer
          /// + last vertex of currently uploaded data.
          cpuBuffers.push_back(CPUBuffer(binding.GetElementSize(), (vertexCount + 2) - batchVertexCount));
          CPUBuffer & cpuBuffer = cpuBuffers.back();
          cpuBuffer.UploadData(rawDataPointer, 1);
        }

        // advance on uploadVertexCount - 1 to copy last vertex also into next VAO with
        // first vertex of data from CPUBuffers
        streams->Advance(batchVertexCount - 1);
      }
    }
  }
}

void TriangleFanBatch::GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const
{
  generate(indexStorage, indexStorage + count, FanIndexGenerator(startIndex));
}

TriangleListOfStripBatch::TriangleListOfStripBatch(BatchCallbacks const & callbacks)
 : TBase(callbacks)
{
}

void TriangleListOfStripBatch::BatchData(ref_ptr<AttributeProvider> streams)
{
  while (streams->IsDataExists())
  {
    ChangeBuffer(true);
    uint16_t const batchVertexCount = BatchIndexes(streams->GetVertexCount());
    FlushData(streams, batchVertexCount);
    streams->Advance(batchVertexCount);
  }
}

uint16_t TriangleListOfStripBatch::VtoICount(uint16_t vCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());
  ASSERT_EQUAL(vCount % vertexStride, 0, ());

  uint16_t const striptCount = vCount / vertexStride;
  return striptCount * TBase::VtoICount(vertexStride);
}

uint16_t TriangleListOfStripBatch::ItoVCount(uint16_t iCount) const
{
  uint8_t const vertexStride = GetVertexStride();
  ASSERT_GREATER_OR_EQUAL(vertexStride, 4, ());
  ASSERT_EQUAL(iCount % 3, 0, ());

  return vertexStride * iCount / TBase::VtoICount(vertexStride);
}

uint16_t TriangleListOfStripBatch::AlignVCount(uint16_t vCount) const
{
  return vCount - vCount % GetVertexStride();
}

void TriangleListOfStripBatch::GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const
{
  uint8_t const vertexStride = GetVertexStride();
  generate(indexStorage, indexStorage + count, ListOfStriptGenerator(startIndex, vertexStride, VtoICount(vertexStride)));
}

} // namespace dp
