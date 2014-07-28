#pragma once

#include "pointers.hpp"

#include "../std/function.hpp"

class AttributeProvider;
class BindingInfo;

class BatchCallbacks
{
public:
  typedef function<void (BindingInfo const &, void const *, uint16_t)> flush_vertex_fn;
  typedef function<uint16_t * (uint16_t, uint16_t &)> get_index_storage_fn;
  typedef function<void ()> submit_index_fn;
  typedef function<uint16_t ()> get_available_fn;
  typedef function<void (bool)> change_buffer_fn;

  flush_vertex_fn      m_flushVertex;
  get_index_storage_fn m_getIndexStorage;
  submit_index_fn      m_submitIndex;
  get_available_fn     m_getAvailableVertex;
  get_available_fn     m_getAvailableIndex;
  change_buffer_fn     m_changeBuffer;
};

class TriangleBatch
{
public:
  TriangleBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(RefPointer<AttributeProvider> streams) = 0;
  void SetIsCanDevideStreams(bool canDevide);
  bool IsCanDevideStreams() const;
  void SetVertexStride(uint8_t vertexStride);

protected:
  void FlushData(RefPointer<AttributeProvider> streams, uint16_t vertexVount) const;
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
  typedef TriangleBatch base_t;

public:
  TriangleListBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(RefPointer<AttributeProvider> streams);
};

class FanStripHelper : public TriangleBatch
{
  typedef TriangleBatch base_t;

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
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const;

private:
  bool m_isFullUploaded;
};

class TriangleStripBatch : public FanStripHelper
{
  typedef FanStripHelper base_t;

public:
  TriangleStripBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(RefPointer<AttributeProvider> streams);
};

class TriangleFanBatch : public FanStripHelper
{
  typedef FanStripHelper base_t;

public:
  TriangleFanBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(RefPointer<AttributeProvider> streams);
};

class TriangleListOfStripBatch : public FanStripHelper
{
  typedef FanStripHelper base_t;

public:
  TriangleListOfStripBatch(BatchCallbacks const & callbacks);

  virtual void BatchData(RefPointer<AttributeProvider> streams);

protected:
  virtual uint16_t VtoICount(uint16_t vCount) const;
  virtual uint16_t ItoVCount(uint16_t iCount) const;
  virtual uint16_t AlignVCount(uint16_t vCount) const;
  virtual void GenerateIndexes(uint16_t * indexStorage, uint16_t count, uint16_t startIndex) const;
};
