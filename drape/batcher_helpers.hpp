#pragma once

#include "drape/pointers.hpp"

#include "std/function.hpp"
#include "std/vector.hpp"

namespace dp
{

class AttributeProvider;
class BindingInfo;

class BatchCallbacks
{
public:
  virtual void FlushData(BindingInfo const & binding, void const * data, uint32_t count) = 0;
  virtual void * GetIndexStorage(uint32_t indexCount, uint32_t & startIndex) = 0;
  virtual void SubmitIndices() = 0;
  virtual uint32_t GetAvailableVertexCount() const = 0;
  virtual uint32_t GetAvailableIndexCount() const = 0;
  virtual void ChangeBuffer() = 0;
};

class UniversalBatch
{
public:
  UniversalBatch(BatchCallbacks & callbacks, uint8_t minVerticesCount, uint8_t minIndicesCount);
  virtual ~UniversalBatch(){}

  virtual void BatchData(ref_ptr<AttributeProvider> streams) = 0;
  void SetCanDevideStreams(bool canDevide);
  bool CanDevideStreams() const;
  void SetVertexStride(uint8_t vertexStride);

protected:
  void FlushData(ref_ptr<AttributeProvider> streams, uint32_t vertexCount) const;
  void FlushData(BindingInfo const & info, void const * data, uint32_t elementCount) const;
  void * GetIndexStorage(uint32_t indexCount, uint32_t & startIndex);
  void SubmitIndex();
  uint32_t GetAvailableVertexCount() const;
  uint32_t GetAvailableIndexCount() const;
  void ChangeBuffer() const;
  uint8_t GetVertexStride() const;

  virtual bool IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const;

private:
  BatchCallbacks & m_callbacks;
  bool m_canDevideStreams;
  uint8_t m_vertexStride;
  uint8_t const m_minVerticesCount;
  uint8_t const m_minIndicesCount;
};

class TriangleListBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  TriangleListBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<AttributeProvider> streams) override;
};

class LineStripBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  LineStripBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<AttributeProvider> streams) override;
};

class LineRawBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  LineRawBatch(BatchCallbacks & callbacks, vector<int> const & indices);

  void BatchData(ref_ptr<AttributeProvider> streams) override;

private:
  vector<int> const & m_indices;
};

class FanStripHelper : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  FanStripHelper(BatchCallbacks & callbacks);

protected:
  uint32_t BatchIndexes(uint32_t vertexCount);
  void CalcBatchPortion(uint32_t vertexCount, uint32_t avVertex, uint32_t avIndex,
                        uint32_t & batchVertexCount, uint32_t & batchIndexCount);
  bool IsFullUploaded() const;

  virtual uint32_t VtoICount(uint32_t vCount) const;
  virtual uint32_t ItoVCount(uint32_t iCount) const;
  virtual uint32_t AlignVCount(uint32_t vCount) const;
  virtual uint32_t AlignICount(uint32_t vCount) const;
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const = 0;

private:
  bool m_isFullUploaded;
};

class TriangleStripBatch : public FanStripHelper
{
  using TBase = FanStripHelper;

public:
  TriangleStripBatch(BatchCallbacks & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
protected:
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const;
};

class TriangleFanBatch : public FanStripHelper
{
  using TBase = FanStripHelper;

public:
  TriangleFanBatch(BatchCallbacks & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
protected:
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const;
};

class TriangleListOfStripBatch : public FanStripHelper
{
  using TBase = FanStripHelper;

public:
  TriangleListOfStripBatch(BatchCallbacks & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);

protected:
  virtual bool IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const;
  virtual uint32_t VtoICount(uint32_t vCount) const;
  virtual uint32_t ItoVCount(uint32_t iCount) const;
  virtual uint32_t AlignVCount(uint32_t vCount) const;
  virtual uint32_t AlignICount(uint32_t iCount) const;
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const;
};

} // namespace dp
