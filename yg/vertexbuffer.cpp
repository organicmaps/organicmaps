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

    VertexBuffer::VertexBuffer(bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA), m_isLocked(false)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
    }

    VertexBuffer::VertexBuffer(size_t size, bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA), m_isLocked(false)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
      resize(size);
    }

    void VertexBuffer::resize(size_t size)
    {
      ASSERT(!m_isLocked, ());
      if (size != m_size)
      {
        m_size = size;
        makeCurrent();
        if (m_useVA)
        {
          delete [] (unsigned char*)m_gpuData;
          m_gpuData = new unsigned char [size];
        }
        else
          OGLCHECK(glBufferData(GL_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));
      }
    }

    size_t VertexBuffer::size() const
    {
      return m_size;
    }

    VertexBuffer::~VertexBuffer()
    {
      if (m_useVA)
        delete [] (unsigned char*)m_gpuData;
      else
        if (g_doDeleteOnDestroy)
          OGLCHECK(glDeleteBuffers(1, &m_id));
    }

    void * VertexBuffer::data()
    {
      ASSERT(m_isLocked, ("IndexBuffer is not locked"));
      return m_gpuData;
    }

    void * VertexBuffer::lock()
    {
      ASSERT(!m_isLocked, ());
      m_isLocked = true;

      if (m_useVA)
        return m_gpuData;

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
      ASSERT(m_isLocked, ());
      m_isLocked = false;

      if (m_useVA)
        return;

      ASSERT(m_gpuData != 0, ("VertexBuffer is not locked"));
      makeCurrent();
#ifdef OMIM_GL_ES
      OGLCHECK(glUnmapBufferOES(GL_ARRAY_BUFFER));
#else
      OGLCHECK(glUnmapBuffer(GL_ARRAY_BUFFER));
#endif
      m_gpuData = 0;
    }

    void * VertexBuffer::glPtr()
    {
      if (m_useVA)
        return m_gpuData;
      return 0;
    }

    bool VertexBuffer::isLocked() const
    {
      return m_isLocked;
    }

    void VertexBuffer::makeCurrent()
    {
      if (m_useVA)
        return;

#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif
        OGLCHECK(glBindBuffer(GL_ARRAY_BUFFER, m_id));
    }

  }
}
