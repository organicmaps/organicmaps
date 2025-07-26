#pragma once

#include "drape/buffer_base.hpp"
#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

namespace dp
{
class GPUBuffer : public BufferBase
{
  using TBase = BufferBase;

public:
  enum Target
  {
    ElementBuffer,
    IndexBuffer
  };

  GPUBuffer(Target t, void const * data, uint8_t elementSize, uint32_t capacity, uint64_t batcherHash);
  ~GPUBuffer() override;

  void UploadData(void const * data, uint32_t elementCount);
  void Bind();

  void * Map(uint32_t elementOffset, uint32_t elementCount);
  void UpdateData(void * gpuPtr, void const * data, uint32_t elementOffset, uint32_t elementCount);
  void Unmap();

protected:
  // Discard old data.
  void Resize(void const * data, uint32_t elementCount);

private:
  Target m_t;
  uint32_t m_bufferID;
  uint32_t m_mappingOffset;
#ifdef TRACK_GPU_MEM
  uint64_t m_batcherHash;
#endif

#ifdef DEBUG
  bool m_isMapped;
#endif
};
}  // namespace dp
