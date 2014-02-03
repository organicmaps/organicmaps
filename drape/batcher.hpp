#pragma once

#include "pointers.hpp"
#include "glstate.hpp"
#include "vertex_array_buffer.hpp"
#include "attribute_provider.hpp"

#include "../std/map.hpp"
#include "../std/function.hpp"

class Batcher
{
public:
  Batcher();
  ~Batcher();

  void InsertTriangleList(const GLState & state, RefPointer<AttributeProvider> params);
  void InsertTriangleStrip(const GLState & state, RefPointer<AttributeProvider> params);
  void InsertTriangleFan(const GLState & state, RefPointer<AttributeProvider> params);

  typedef function<void (const GLState &, TransferPointer<VertexArrayBuffer> )> flush_fn;
  void StartSession(const flush_fn & flusher);
  void EndSession();

private:
  RefPointer<VertexArrayBuffer> GetBuffer(const GLState & state);
  /// return true if GLBuffer is finished
  bool UploadBufferData(RefPointer<GPUBuffer> vertexBuffer, RefPointer<AttributeProvider> params);
  void FinalizeBuffer(const GLState & state);
  void Flush();

private:
  flush_fn m_flushInterface;

private:
  typedef map<GLState, MasterPointer<VertexArrayBuffer> > buckets_t;
  buckets_t m_buckets;
};
