#pragma once

#include "drape/pointers.hpp"
#include "drape/buffer_base.hpp"

namespace dp
{

class GPUBuffer : public BufferBase
{
  typedef BufferBase TBase;
public:
  enum Target
  {
    ElementBuffer,
    IndexBuffer
  };

public:
  GPUBuffer(Target t, uint8_t elementSize, uint16_t capacity);
  GPUBuffer(Target t, void const * data, uint8_t elementSize, uint16_t capacity);
  ~GPUBuffer();

  void UploadData(void const * data, uint16_t elementCount);
  void Bind();
  void Seek(uint16_t elementNumber);

protected:
  void * Map();
  void UpdateData(void * gpuPtr, void const * data, uint16_t elementOffset, uint16_t elementCount);
  void Unmap();

  /// discard old data
  void Resize(void const * data, uint16_t elementCount);

private:
  friend class GPUBufferMapper;
  Target m_t;
  uint32_t m_bufferID;

#ifdef DEBUG
  bool m_isMapped;
#endif
};

class GPUBufferMapper
{
public:
  GPUBufferMapper(RefPointer<GPUBuffer> buffer);
  ~GPUBufferMapper();

  void UpdateData(void const * data, uint16_t elementOffset, uint16_t elementCount);

private:
#ifdef DEBUG
  static uint32_t m_mappedDataBuffer;
  static uint32_t m_mappedIndexBuffer;
#endif

  RefPointer<GPUBuffer> m_buffer;
  void * m_gpuPtr;
};

} // namespace dp
