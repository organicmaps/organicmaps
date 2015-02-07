#include "drape/gpu_buffer.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glextensions_list.hpp"
#include "drape/utils/gpu_mem_tracker.hpp"

#include "base/assert.hpp"

#include "std/cstring.hpp"

#define CHECK_VBO_BOUNDS

namespace dp
{

namespace
{
  bool IsMapBufferSupported()
  {
    static bool const isSupported = GLExtensionsList::Instance().IsSupported(GLExtensionsList::MapBuffer);
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
  : TBase(elementSize, capacity)
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
  GLFunctions::glBindBuffer(0, glTarget(m_t));
  GLFunctions::glDeleteBuffer(m_bufferID);

#if defined(TRACK_GPU_MEM)
  dp::GPUMemTracker::Inst().RemoveDeallocated("VBO", m_bufferID);
#endif
}

void GPUBuffer::UploadData(void const * data, uint16_t elementCount)
{
  ASSERT(m_isMapped == false, ());

  uint16_t currentSize = GetCurrentSize();
  uint8_t elementSize = GetElementSize();
  ASSERT(GetCapacity() >= elementCount + currentSize, ("Not enough memory to upload ", elementCount, " elements"));
  Bind();

#if defined(CHECK_VBO_BOUNDS)
  int32_t size = GLFunctions::glGetBufferParameter(glTarget(m_t), gl_const::GLBufferSize);
  ASSERT_EQUAL(GetCapacity() * elementSize, size, ());
  ASSERT_LESS_OR_EQUAL((elementCount + currentSize) * elementSize, size,());
#endif

  GLFunctions::glBufferSubData(glTarget(m_t), elementCount * elementSize, data, currentSize * elementSize);
  TBase::UploadData(elementCount);

#if defined(TRACK_GPU_MEM)
  dp::GPUMemTracker::Inst().SetUsed("VBO", m_bufferID, (currentSize + elementCount) * elementSize);
#endif
}

void GPUBuffer::Bind()
{
  GLFunctions::glBindBuffer(m_bufferID, glTarget(m_t));
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
  uint32_t const elementSize = static_cast<uint32_t>(GetElementSize());
  uint32_t const byteOffset = elementOffset * elementSize;
  uint32_t const byteCount = elementCount * elementSize;
  uint32_t const byteCapacity = GetCapacity() * elementSize;
  ASSERT(m_isMapped == true, ());

#if defined(CHECK_VBO_BOUNDS)
  int32_t size = GLFunctions::glGetBufferParameter(glTarget(m_t), gl_const::GLBufferSize);
  ASSERT_EQUAL(size, byteCapacity, ());
  ASSERT_LESS(byteOffset + byteCount, size, ());
#endif

  if (IsMapBufferSupported())
  {
    ASSERT(gpuPtr != NULL, ());
    memcpy((uint8_t *)gpuPtr + byteOffset, data, byteCount);
  }
  else
  {
    ASSERT(gpuPtr == NULL, ());
    if (byteOffset == 0 && byteCount == byteCapacity)
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
  TBase::Resize(elementCount);
  Bind();
  GLFunctions::glBufferData(glTarget(m_t), GetCapacity() * GetElementSize(), NULL, gl_const::GLDynamicDraw);
#if defined(TRACK_GPU_MEM)
  dp::GPUMemTracker & memTracker = dp::GPUMemTracker::Inst();
  memTracker.RemoveDeallocated("VBO", m_bufferID);
  memTracker.AddAllocated("VBO", m_bufferID, GetCapacity() * GetElementSize());
#endif
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

} // namespace dp
