#include "gpu_buffer.hpp"
#include "glfunctions.hpp"
#include "glextensions_list.hpp"

#include "../base/assert.hpp"

namespace
{
  bool IsMapBufferSupported()
  {
    static bool isSupported = GLExtensionsList::Instance().IsSupported(GLExtensionsList::MapBuffer);
    return isSupported;
  }
}

glConst glTarget(GPUBuffer::Target t)
{
  if (t == GPUBuffer::ElementBuffer)
    return gl_const::GLArrayBuffer;

  return gl_const::GLElementArrayBuffer;
}

GPUBuffer::GPUBuffer(Target t, uint8_t elementSize, uint16_t capacity)
  : base_t(elementSize, capacity)
  , m_t(t)
#ifdef DEBUG
  , m_isMapped(false)
#endif
{
  m_bufferID = GLFunctions::glGenBuffer();
  Resize(capacity);
}

GPUBuffer::~GPUBuffer()
{
  GLFunctions::glDeleteBuffer(m_bufferID);
}

void GPUBuffer::UploadData(void const * data, uint16_t elementCount)
{
  ASSERT(m_isMapped == false, ());

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

void * GPUBuffer::Map()
{
#ifdef DEBUG
  ASSERT(m_isMapped == false, ());
  m_isMapped = true;
#endif

  if (IsMapBufferSupported())
    return GLFunctions::glMapBuffer(glTarget(m_t));

  return NULL;
}

void GPUBuffer::UpdateData(void * gpuPtr, void const * data, uint16_t elementOffset, uint16_t elementCount)
{
  uint16_t elementSize = GetElementSize();
  uint32_t byteOffset = elementOffset * (uint32_t)elementSize;
  uint32_t byteCount = elementCount * (uint32_t)elementSize;
  ASSERT(m_isMapped == true, ());
  if (IsMapBufferSupported())
  {
    ASSERT(gpuPtr != NULL, ());
    memcpy((uint8_t *)gpuPtr + byteOffset, data, byteCount);
  }
  else
  {
    ASSERT(gpuPtr == NULL, ());
    if (byteOffset == 0 && byteCount == GetCapacity())
      GLFunctions::glBufferData(glTarget(m_t), byteCount, data, gl_const::GLStaticDraw);
    else
      GLFunctions::glBufferSubData(glTarget(m_t), byteCount, data, byteOffset);
  }
}

void GPUBuffer::Unmap()
{
#ifdef DEBUG
  ASSERT(m_isMapped == true, ());
  m_isMapped = false;
#endif
  if (IsMapBufferSupported())
    GLFunctions::glUnmapBuffer(glTarget(m_t));
}

void GPUBuffer::Resize(uint16_t elementCount)
{
  base_t::Resize(elementCount);
  Bind();
  GLFunctions::glBufferData(glTarget(m_t), GetCapacity() * GetElementSize(), NULL, gl_const::GLStaticDraw);
}

////////////////////////////////////////////////////////////////////////////
GPUBufferMapper::GPUBufferMapper(RefPointer<GPUBuffer> buffer)
  : m_buffer(buffer)
{
#ifdef DEBUG
  if (m_buffer->m_t == GPUBuffer::ElementBuffer)
  {
    ASSERT(m_mappedDataBuffer == 0, ());
    m_mappedDataBuffer = m_buffer->m_bufferID;
  }
  else
  {
    ASSERT(m_mappedIndexBuffer == 0, ());
    m_mappedIndexBuffer = m_buffer->m_bufferID;
  }
#endif

  m_buffer->Bind();
  m_gpuPtr = m_buffer->Map();
}

GPUBufferMapper::~GPUBufferMapper()
{
#ifdef DEBUG
  if (m_buffer->m_t == GPUBuffer::ElementBuffer)
  {
    ASSERT(m_mappedDataBuffer == m_buffer->m_bufferID, ());
    m_mappedDataBuffer = 0;
  }
  else
  {
    ASSERT(m_mappedIndexBuffer == m_buffer->m_bufferID, ());
    m_mappedIndexBuffer = 0;
  }
#endif

  m_buffer->Unmap();
}

void GPUBufferMapper::UpdateData(void const * data, uint16_t elementOffset, uint16_t elementCount)
{
  m_buffer->UpdateData(m_gpuPtr, data, elementOffset, elementCount);
}

#ifdef DEBUG
  uint32_t GPUBufferMapper::m_mappedDataBuffer;
  uint32_t GPUBufferMapper::m_mappedIndexBuffer;
#endif
