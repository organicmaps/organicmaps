#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"
#include "../base/assert.hpp"

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

    void IndexBuffer::pushCurrent()
    {
//      indexBufferStack.push_back(current());
    }

    void IndexBuffer::popCurrent()
    {
//      if (indexBufferStack.back() != (unsigned int)-1)
//        OGLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferStack.back()));
//      indexBufferStack.pop_back();
    }

    IndexBuffer::IndexBuffer(bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
    }

    IndexBuffer::IndexBuffer(size_t size, bool useVA)
      : m_size(0), m_gpuData(0), m_useVA(useVA)
    {
      if (!m_useVA)
        OGLCHECK(glGenBuffers(1, &m_id));
      resize(size);
    }

    void IndexBuffer::resize(size_t size)
    {
      if (size != m_size)
      {
        m_size = size;
        makeCurrent();
        if (m_useVA)
        {
          delete [] (unsigned char*) m_gpuData;
          m_gpuData = new unsigned char[size];
        }
        else
          OGLCHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_size, 0, GL_DYNAMIC_DRAW));
      }
    }

    size_t IndexBuffer::size() const
    {
      return m_size;
    }

    IndexBuffer::~IndexBuffer()
    {
      if (!m_useVA)
        OGLCHECK(glDeleteBuffers(1, &m_id));
    }

    void * IndexBuffer::lock()
    {
      if (m_useVA)
        return m_gpuData;

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

    void IndexBuffer::makeCurrent()
    {
      if (m_useVA)
        return;

      //if (m_id != current())
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

