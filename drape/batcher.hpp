#pragma once

#include "glstate.hpp"
#include "vertex_array_buffer.hpp"
#include "attribute_provider.hpp"

#include "../std/map.hpp"

class IBatchFlush
{
public:
  virtual ~IBatchFlush() {}

  virtual void FlushFullBucket(const GLState & state, StrongPointer<VertexArrayBuffer> backet) = 0;
  virtual void UseIncompleteBucket(const GLState & state, WeakPointer<VertexArrayBuffer> backet) = 0;
};

class Batcher
{
public:
  Batcher(WeakPointer<IBatchFlush> flushInterface);
  ~Batcher();

  void InsertTriangleList(const GLState & state, WeakPointer<AttributeProvider> params);
  void InsertTriangleStrip(const GLState & state, WeakPointer<AttributeProvider> params);
  void InsertTriangleFan(const GLState & state, WeakPointer<AttributeProvider> params);

  void RequestIncompleteBuckets();

private:
  template <typename strategy>
  void InsertTriangles(const GLState & state, strategy s, WeakPointer<AttributeProvider> params);

  WeakPointer<VertexArrayBuffer> GetBuffer(const GLState & state);
  /// return true if GLBuffer is finished
  bool UploadBufferData(WeakPointer<GLBuffer> vertexBuffer, WeakPointer<AttributeProvider> params);
  void FinalizeBuffer(const GLState & state);

private:
  WeakPointer<IBatchFlush> m_flushInterface;

private:
  typedef map<GLState, StrongPointer<VertexArrayBuffer> > buckets_t;
  buckets_t m_buckets;
};
