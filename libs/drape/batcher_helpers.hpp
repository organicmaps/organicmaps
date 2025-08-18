#pragma once

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"

#include <vector>

namespace dp
{
class AttributeProvider;
class BindingInfo;

class BatchCallbacks
{
public:
  virtual ~BatchCallbacks() = default;
  virtual void FlushData(ref_ptr<GraphicsContext> context, BindingInfo const & binding, void const * data,
                         uint32_t count) = 0;
  virtual void * GetIndexStorage(uint32_t indexCount, uint32_t & startIndex) = 0;
  virtual void SubmitIndices(ref_ptr<GraphicsContext> context) = 0;
  virtual uint32_t GetAvailableVertexCount() const = 0;
  virtual uint32_t GetAvailableIndexCount() const = 0;
  virtual void ChangeBuffer(ref_ptr<GraphicsContext> context) = 0;
};

class UniversalBatch
{
public:
  UniversalBatch(BatchCallbacks & callbacks, uint8_t minVerticesCount, uint8_t minIndicesCount);
  virtual ~UniversalBatch() = default;

  virtual void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) = 0;
  void SetCanDivideStreams(bool canDivide);
  bool CanDivideStreams() const;
  void SetVertexStride(uint8_t vertexStride);

protected:
  void FlushData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams, uint32_t vertexCount) const;
  void FlushData(ref_ptr<GraphicsContext> context, BindingInfo const & info, void const * data,
                 uint32_t elementCount) const;
  void * GetIndexStorage(uint32_t indexCount, uint32_t & startIndex);
  void SubmitIndices(ref_ptr<GraphicsContext> context);
  uint32_t GetAvailableVertexCount() const;
  uint32_t GetAvailableIndexCount() const;
  void ChangeBuffer(ref_ptr<GraphicsContext> context) const;
  uint8_t GetVertexStride() const;

  virtual bool IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const;

private:
  BatchCallbacks & m_callbacks;
  bool m_canDivideStreams;
  uint8_t m_vertexStride;
  uint8_t const m_minVerticesCount;
  uint8_t const m_minIndicesCount;
};

class TriangleListBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  explicit TriangleListBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;
};

class LineStripBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  explicit LineStripBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;
};

class LineRawBatch : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  LineRawBatch(BatchCallbacks & callbacks, std::vector<int> const & indices);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;

private:
  std::vector<int> const & m_indices;
};

class FanStripHelper : public UniversalBatch
{
  using TBase = UniversalBatch;

public:
  explicit FanStripHelper(BatchCallbacks & callbacks);

protected:
  uint32_t BatchIndexes(ref_ptr<GraphicsContext> context, uint32_t vertexCount);
  void CalcBatchPortion(uint32_t vertexCount, uint32_t avVertex, uint32_t avIndex, uint32_t & batchVertexCount,
                        uint32_t & batchIndexCount);
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
  explicit TriangleStripBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;

protected:
  void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const override;
};

class TriangleFanBatch : public FanStripHelper
{
  using TBase = FanStripHelper;

public:
  explicit TriangleFanBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;

protected:
  void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const override;
};

class TriangleListOfStripBatch : public FanStripHelper
{
  using TBase = FanStripHelper;

public:
  explicit TriangleListOfStripBatch(BatchCallbacks & callbacks);

  void BatchData(ref_ptr<GraphicsContext> context, ref_ptr<AttributeProvider> streams) override;

protected:
  bool IsBufferFilled(uint32_t availableVerticesCount, uint32_t availableIndicesCount) const override;
  uint32_t VtoICount(uint32_t vCount) const override;
  uint32_t ItoVCount(uint32_t iCount) const override;
  uint32_t AlignVCount(uint32_t vCount) const override;
  uint32_t AlignICount(uint32_t iCount) const override;
  void GenerateIndexes(void * indexStorage, uint32_t count, uint32_t startIndex) const override;
};
}  // namespace dp
