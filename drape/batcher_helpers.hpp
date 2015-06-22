#pragma once

#include "drape/pointers.hpp"

#include "std/function.hpp"

namespace dp
{

class AttributeProvider;
class BindingInfo;

class BatchCallbacks
{
public:
  typedef function<void (BindingInfo const &, void const *, uint32_t)> TFlushVertexFn;
  typedef function<void * (uint32_t, uint32_t &)> TGetIndexStorageFn;
  typedef function<void ()> TSubmitIndexFn;
  typedef function<uint32_t ()> TGetAvailableFn;
  typedef function<void ()> ChangeBufferFn;

  TFlushVertexFn      m_flushVertex;
  TGetIndexStorageFn m_getIndexStorage;
  TSubmitIndexFn      m_submitIndex;
  TGetAvailableFn     m_getAvailableVertex;
  TGetAvailableFn     m_getAvailableIndex;
  ChangeBufferFn     m_changeBuffer;
};

class TriangleBatch
{
public:
  TriangleBatch(BatchCallbacks const & callbacks);
  virtual ~TriangleBatch(){}

  virtual void BatchData(ref_ptr<AttributeProvider> streams) = 0;
  void SetIsCanDevideStreams(bool canDevide);
  bool IsCanDevideStreams() const;
  void SetVertexStride(uint8_t vertexStride);

protected:
  void FlushData(ref_ptr<AttributeProvider> streams, uint32_t vertexVount) const;
  void FlushData(BindingInfo const & info, void const * data, uint32_t elementCount) const;
  void * GetIndexStorage(uint32_t indexCount, uint32_t & startIndex);
  void SubmitIndex();
  uint32_t GetAvailableVertexCount() const;
  uint32_t GetAvailableIndexCount() const;
  void ChangeBuffer() const;
  uint8_t GetVertexStride() const;

  virtual bool IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const;

private:
  BatchCallbacks m_callbacks;
  bool m_canDevideStreams;
  uint8_t m_vertexStride;
};

class TriangleListBatch : public TriangleBatch
{
  typedef TriangleBatch TBase;

public:
  TriangleListBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
};

class FanStripHelper : public TriangleBatch
{
  typedef TriangleBatch TBase;

public:
  FanStripHelper(BatchCallbacks const & callbacks);

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
  typedef FanStripHelper TBase;

public:
  TriangleStripBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
protected:
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const;
};

class TriangleFanBatch : public FanStripHelper
{
  typedef FanStripHelper TBase;

public:
  TriangleFanBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
protected:
  virtual void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const;
};

class TriangleListOfStripBatch : public FanStripHelper
{
  typedef FanStripHelper TBase;

public:
  TriangleListOfStripBatch(BatchCallbacks const & callbacks);

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
