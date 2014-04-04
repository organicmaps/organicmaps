#include "gpu_buffer.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

glConst glTarget(GPUBuffer::Target t)
{
  if (t == GPUBuffer::ElementBuffer)
    return GLConst::GLArrayBuffer;

  return GLConst::GLElementArrayBuffer;
}

GPUBuffer::GPUBuffer(Target t, uint8_t elementSize, uint16_t capacity)
  : base_t(elementSize, capacity)
  , m_t(t)
{
  m_bufferID = GLFunctions::glGenBuffer();
  Bind();
  Resize(capacity);
}

GPUBuffer::~GPUBuffer()
{
  GLFunctions::glDeleteBuffer(m_bufferID);
}

void GPUBuffer::UploadData(void const * data, uint16_t elementCount)
{
  uint16_t currentSize = GetCurrentSize();
  uint8_t elementSize = GetElementSize();
  ASSERT(GetCapacity() >= elementCount + currentSize, ("Not enough memory to upload ", elementCount, " elements"));
  Bind();
  GLFunctions::glBufferSubData(glTarget(m_t), elementCount * elementSize, data, currentSize * elementSize);
  base_t::UploadData(elementCount);
}

void GPUBuffer::Bind()
{
  GLFunctions::glBindBuffer(m_bufferID, glTarget((m_t)));
}

void GPUBuffer::Resize(uint16_t elementCount)
{
  base_t::Resize(elementCount);
  GLFunctions::glBufferData(glTarget(m_t), GetCapacity() * GetElementSize(), NULL, GLConst::GLStaticDraw);
}
