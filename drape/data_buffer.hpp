#pragma once

#include "drape/cpu_buffer.hpp"
#include "drape/gpu_buffer.hpp"

namespace dp
{

/// This class works as proxy. It contains CPU-buffer or GPU-buffer inside at a moment
/// and redirects invocations of methods to one or another buffer. Initially it's configured
/// as CPU-buffer and is able to move data from CPU to GPU only once.
class DataBuffer
{
public:
  DataBuffer(GPUBuffer::Target target, uint8_t elementSize, uint16_t capacity);
  ~DataBuffer();

  uint16_t GetCapacity() const;
  uint16_t GetCurrentSize() const;
  uint16_t GetAvailableSize() const;

  void UploadData(void const * data, uint16_t elementCount);
  void Seek(uint16_t elementNumber);
  void Bind();
  void MoveToGPU();

  dp::RefPointer<GPUBuffer> GetGpuBuffer() const;
  dp::RefPointer<CPUBuffer> GetСpuBuffer() const;

protected:
  BufferBase const * GetActiveBuffer() const;

private:
  dp::MasterPointer<GPUBuffer> m_gpuBuffer;
  dp::MasterPointer<CPUBuffer> m_cpuBuffer;
  GPUBuffer::Target m_target;
};

} // namespace dp

