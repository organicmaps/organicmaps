#pragma once

#include "overlay_handle.hpp"
#include "vertex_array_buffer.hpp"
#include "uniform_value.hpp"
#include "uniform_values_storage.hpp"

class RenderBucket
{
public:
  RenderBucket(TransferPointer<VertexArrayBuffer> buffer);
  ~RenderBucket();

  RefPointer<VertexArrayBuffer> GetBuffer();

  void AddOverlayHandle(TransferPointer<OverlayHandle> handle);
  void InsertUniform(TransferPointer<UniformValue> uniform);

  void CollectOverlayHandles(/*OverlayTree */);
  void Render();

private:
  vector<MasterPointer<OverlayHandle> > m_overlay;
  MasterPointer<VertexArrayBuffer> m_buffer;
  UniformValuesStorage m_uniforms;
};
