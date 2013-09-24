#include "glbuffer.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

glConst glTarget(GLBuffer::Target t)
{
  if (t == GLBuffer::ElementBuffer)
    return GLConst::GLArrayBuffer;

  return GLConst::GLElementArrayBuffer;
}

GLBuffer::GLBuffer(Target t, uint8_t elementSize, uint16_t capacity)
  : m_t(t)
  , m_elementSize(elementSize)
  , m_capacity(capacity)
{
  m_size = 0;
  m_bufferID = GLFunctions::glGenBuffer();
  Bind();
  GLFunctions::glBufferData(glTarget(m_t), m_capacity * m_elementSize, NULL, GLConst::GLStaticDraw);
  Unbind();
}

GLBuffer::~GLBuffer()
{
  Unbind();
  GLFunctions::glDeleteBuffer(m_bufferID);
}

void GLBuffer::UploadData(const void * data, uint16_t elementCount)
{
  ASSERT(m_capacity >= elementCount + m_size, ("Not enough memory to upload ", elementCount, " elements"));
  Bind();
  GLFunctions::glBufferSubData(glTarget(m_t), elementCount * m_elementSize, data, m_size * m_elementSize);
  m_size += elementCount;
  Unbind();
}

uint16_t GLBuffer::GetCapacity() const
{
  return m_capacity;
}

uint16_t GLBuffer::GetCurrentSize() const
{
  return m_size;
}

uint16_t GLBuffer::GetAvailableSize() const
{
  return m_capacity - m_size;
}

void GLBuffer::Bind()
{
  GLFunctions::glBindBuffer(m_bufferID, glTarget((m_t)));
}

void GLBuffer::Unbind()
{
  GLFunctions::glBindBuffer(0, glTarget((m_t)));
}
