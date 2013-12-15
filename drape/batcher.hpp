#pragma once

#include "pointers.hpp"
#include "glstate.hpp"
#include "vertex_array_buffer.hpp"
#include "attribute_provider.hpp"

#include "../std/map.hpp"

class IBatchFlush
{
public:
  virtual ~IBatchFlush() {}

  virtual void FlushFullBucket(const GLState & state, TransferPointer<VertexArrayBuffer> backet) = 0;
};

class Batcher
{
public:
  Batcher(RefPointer<IBatchFlush> flushInterface);
  ~Batcher();

  void InsertTriangleList(const GLState & state, RefPointer<AttributeProvider> params);
  void InsertTriangleStrip(const GLState & state, RefPointer<AttributeProvider> params);
  void InsertTriangleFan(const GLState & state, RefPointer<AttributeProvider> params);
  void Flush();

private:
  template <typename strategy>
  void InsertTriangles(const GLState & state, strategy s, RefPointer<AttributeProvider> params);

  RefPointer<VertexArrayBuffer> GetBuffer(const GLState & state);
  /// return true if GLBuffer is finished
  bool UploadBufferData(RefPointer<GLBuffer> vertexBuffer, RefPointer<AttributeProvider> params);
  void FinalizeBuffer(const GLState & state);

private:
  RefPointer<IBatchFlush> m_flushInterface;

private:
  typedef map<GLState, MasterPointer<VertexArrayBuffer> > buckets_t;
  buckets_t m_buckets;
};
