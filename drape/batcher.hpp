#pragma once

#include "glstate.hpp"
#include "vertex_array_buffer.hpp"
#include "attribute_provider.hpp"

#include "../std/map.hpp"

class IBatchFlush
{
public:
  virtual ~IBatchFlush() {}

  virtual void FlushFullBucket(const GLState & state, OwnedPointer<VertexArrayBuffer> backet) = 0;
  virtual void UseIncompleteBucket(const GLState & state, ReferencePoiner<VertexArrayBuffer> backet) = 0;
};

class Batcher
{
public:
  Batcher(ReferencePoiner<IBatchFlush> flushInterface);
  ~Batcher();

  void InsertTriangleList(const GLState & state, ReferencePoiner<AttributeProvider> params);
  void InsertTriangleStrip(const GLState & state, ReferencePoiner<AttributeProvider> params);
  void InsertTriangleFan(const GLState & state, ReferencePoiner<AttributeProvider> params);

  void RequestIncompleteBuckets();

private:
  template <typename strategy>
  void InsertTriangles(const GLState & state, strategy s, ReferencePoiner<AttributeProvider> params);

  ReferencePoiner<VertexArrayBuffer> GetBuffer(const GLState & state);
  /// return true if GLBuffer is finished
  bool UploadBufferData(ReferencePoiner<GLBuffer> vertexBuffer, ReferencePoiner<AttributeProvider> params);
  void FinalizeBuffer(const GLState & state);

private:
  ReferencePoiner<IBatchFlush> m_flushInterface;

private:
  typedef map<GLState, OwnedPointer<VertexArrayBuffer> > buckets_t;
  buckets_t m_buckets;
};
