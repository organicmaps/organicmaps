#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"
#include "../base/assert.hpp"

#include "internal/opengl.hpp"

#include "vertexbuffer.hpp"
#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    list<unsigned int> vertexBufferStack;

    unsigned VertexBuffer::current()
    {
      int id;
      OGLCHECK(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &id));
      return id;
    }

    void VertexBuffer::pushCurrent()
    {
      vertexBufferStack.push_back(current());
    }

    void VertexBuffer::popCurrent()
    {
      if (vertexBufferStack.back() != (unsigned int)-1)
        OGLCHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBufferStack.back()));
      vertexBufferStack.pop_back();
    }

    VertexBuffer::VertexBuffer() : m_size(0), m_gpuData(0)
    {
      OGLCHECK(glGenBuffers(1, &m_id));
    }

    VertexBuffer::VertexBuffer(size_t size) : m_size(0), m_gpuData(0)
    {
      OGLCHECK(glGenBuffers(1, &m_id));
      resize(size);
    }

    void VertexBuffer::resize(size_t size)
    {
      if (size != m_size)
      {
        m_size = size;
        makeCurrent();
        OGLCHECK(glBufferData(GL_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));
      }
    }

    size_t VertexBuffer::size() const
    {
      return m_size;
    }

    VertexBuffer::~VertexBuffer()
    {
      OGLCHECK(glDeleteBuffers(1, &m_id));
    }

    void * VertexBuffer::lock()
    {
      makeCurrent();

      /// orphaning the old copy of the buffer data.
      /// this provides that the glMapBuffer will not wait.
      OGLCHECK(glBufferData(GL_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));

#ifdef OMIM_GL_ES
      m_gpuData = glMapBufferOES(GL_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
#else
      m_gpuData = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
#endif
      OGLCHECKAFTER;
      return m_gpuData;
    }

    void VertexBuffer::unlock()
    {
      ASSERT(m_gpuData != 0, ("VertexBuffer is not locked"));
      makeCurrent();
#ifdef OMIM_GL_ES
      OGLCHECK(glUnmapBufferOES(GL_ARRAY_BUFFER));
#else
      OGLCHECK(glUnmapBuffer(GL_ARRAY_BUFFER));
#endif
      m_gpuData = 0;
    }

    void VertexBuffer::makeCurrent()
    {
      if (m_id != current())
        OGLCHECK(glBindBuffer(GL_ARRAY_BUFFER, m_id));
    }

  }
}
