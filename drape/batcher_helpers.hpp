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
  typedef function<void (BindingInfo const &, void const *, uint16_t)> TFlushVertexFn;
  typedef function<uint16_t * (uint16_t, uint16_t &)> TGetIndexStorageFn;
  typedef function<void ()> TSubmitIndexFn;
  typedef function<uint16_t ()> TGetAvailableFn;
  typedef function<void (bool)> ChangeBufferFn;

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

  virtual void BatchData(ref_ptr<AttributeProvider> streams) = 0;
  void SetIsCanDevideStreams(bool canDevide);
  bool IsCanDevideStreams() const;
  void SetVertexStride(uint8_t vertexStride);

protected:
  void FlushData(ref_ptr<AttributeProvider> streams, uint16_t vertexVount) const;
  void FlushData(BindingInfo const & info, void const * data, uint16_t elementCount) const;
  uint16_t * GetIndexStorage(uint16_t indexCount, uint16_t & startIndex);
  void SubmitIndex();
  uint16_t GetAvailableVertexCount() const;
  uint16_t GetAvailableIndexCount() const;
  void ChangeBuffer(bool checkFilled) const;
  uint8_t GetVertexStride() const;


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
  uint16_t BatchIndexes(uint16_t vertexCount);
  void CalcBatchPortion(uint16_t vertexCount, uint16_t avVertex, uint16_t avIndex,
                        uint16_t & batchVertexCount, uint16_t & batchIndexCount);
  bool IsFullUploaded() const;

  virtual uint16_t VtoICount(uint16_t vCount) const;
  virtual uint16_t ItoVCount(uint16_t iCount) const;
  virtual uint16_t AlignVCount(uint16_t vCount) const;
  virtual uint16_t AlignICount(uint16_t vCount) const;
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const = 0;

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
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const;
};

class TriangleFanBatch : public FanStripHelper
{
  typedef FanStripHelper TBase;

public:
  TriangleFanBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);
protected:
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const;
};

class TriangleListOfStripBatch : public FanStripHelper
{
  typedef FanStripHelper TBase;

public:
  TriangleListOfStripBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(ref_ptr<AttributeProvider> streams);

protected:
  virtual uint16_t VtoICount(uint16_t vCount) const;
  virtual uint16_t ItoVCount(uint16_t iCount) const;
  virtual uint16_t AlignVCount(uint16_t vCount) const;
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const;
};

} // namespace dp
