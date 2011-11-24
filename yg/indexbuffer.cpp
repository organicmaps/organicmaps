#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"
#include "../base/assert.hpp"
#include "../base/shared_buffer_manager.hpp"

#include "internal/opengl.hpp"

#include "indexbuffer.hpp"
#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    list<unsigned int> indexBufferStack;

    int IndexBuffer::current()
    {
      int id;
      OGLCHECK(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &id));
      return id;
    }

    IndexBuffer::IndexBuffer(bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA), m_isLocked(false)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
    }

    IndexBuffer::IndexBuffer(size_t size, bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA), m_isLocked(false)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
      resize(size);
    }

    void IndexBuffer::resize(size_t size)
    {
      ASSERT(!m_isLocked, ());
      if (size != m_size)
      {
        discard();
        m_size = size;
        makeCurrent();
        if (!m_useVA)
          OGLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));
      }
    }

    size_t IndexBuffer::size() const
    {
      return m_size;
    }

    IndexBuffer::~IndexBuffer()
    {
      if ((!m_useVA) && (g_doDeleteOnDestroy))
        OGLCHECK(glDeleteBuffers(1, &m_id));
    }

    bool IndexBuffer::isLocked() const
    {
      return m_isLocked;
    }

    void * IndexBuffer::data()
    {
      ASSERT(m_isLocked, ("IndexBuffer is not locked"));
      return m_gpuData;
    }

    void * IndexBuffer::lock()
    {
      ASSERT(!m_isLocked, ());
      m_isLocked = true;

      if (m_useVA)
      {
        if (!m_sharedBuffer)
          m_sharedBuffer = SharedBufferManager::instance().reserveSharedBuffer(m_size);

        m_gpuData = &m_sharedBuffer->at(0);
        return m_gpuData;
      }

      makeCurrent();

      /// orphaning the old copy of the buffer data.
      /// this provides that the glMapBuffer will not wait.
      OGLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));

#ifdef OMIM_GL_ES
      m_gpuData = glMapBufferOES(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
#else
      m_gpuData = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
#endif
      OGLCHECKAFTER;
      return m_gpuData;
    }

    void IndexBuffer::unlock()
    {
      ASSERT(m_isLocked, ());
      m_isLocked = false;

      if (m_useVA)
        return;

      ASSERT(m_gpuData != 0, ("IndexBuffer is not locked"));
      makeCurrent();
#ifdef OMIM_GL_ES
      OGLCHECK(glUnmapBufferOES(GL_ELEMENT_ARRAY_BUFFER));
#else
      OGLCHECK(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));
#endif
      m_gpuData = 0;
    }

    void IndexBuffer::discard()
    {
      if (m_useVA)
      {
        if (m_sharedBuffer)
        {
          SharedBufferManager::instance().freeSharedBuffer(m_size, m_sharedBuffer);
          m_sharedBuffer.reset();
          m_gpuData = 0;
        }
      }
    }

    void IndexBuffer::makeCurrent()
    {
      if (m_useVA)
        return;

#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif
        OGLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id));
    }

    void * IndexBuffer::glPtr()
    {
      if (m_useVA)
        return m_gpuData;
      else
        return 0;
    }
  }
}

