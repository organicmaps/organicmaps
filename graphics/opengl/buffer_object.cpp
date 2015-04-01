#include "buffer_object.hpp"
#include "opengl.hpp"

#include "../../base/assert.hpp"
#include "../../base/logging.hpp"
#include "../../base/macros.hpp"
#include "../../base/shared_buffer_manager.hpp"
#include "../../base/SRC_FIRST.hpp"

#include "../../std/list.hpp"


namespace graphics
{
  namespace gl
  {
    void BufferObject::Binder::Reset(BufferObject * bufferObj)
    {
      ASSERT(bufferObj, ());
      ASSERT(!bufferObj->IsBound(), ());

      m_bufferObj = bufferObj;
      bufferObj->Bind();
    }

    BufferObject::Binder::~Binder()
    {
      if (m_bufferObj != nullptr)
        m_bufferObj->Unbind();
    }

    BufferObject::BufferObject(unsigned target)
      : m_target(target),
        m_size(0),
        m_gpuData(0),
        m_isLocked(false),
        m_isUsingMapBuffer(false)
    {
      if (g_isBufferObjectsSupported)
        OGLCHECK(glGenBuffersFn(1, &m_id));
    }

    BufferObject::BufferObject(size_t size, unsigned target)
      : m_target(target),
        m_size(0),
        m_gpuData(0),
        m_isLocked(false),
        m_isUsingMapBuffer(false),
        m_bound(false)
    {
      if (g_isBufferObjectsSupported)
        OGLCHECK(glGenBuffersFn(1, &m_id));
      resize(size);
    }

    void BufferObject::resize(size_t size)
    {
      ASSERT(!m_isLocked, ());

      if (size != m_size)
      {
        discard();

        m_size = size;

        /// When binder leaves the scope the buffer object will be unbound.
        Binder binder;
        makeCurrent(binder);
        if (g_isBufferObjectsSupported)
        {
          OGLCHECK(glBufferDataFn(m_target, m_size, 0, GL_DYNAMIC_DRAW));
          /// In multithreaded resource usage scenarios the suggested way to see
          /// resource update made in one thread to the another thread is
          /// to call the glFlush in thread, which modifies resource and then rebind
          /// resource in another threads that is using this resource, if any.
          OGLCHECK(glFlushFn());
        }
      }
    }

    size_t BufferObject::size() const
    {
      return m_size;
    }

    BufferObject::~BufferObject()
    {
      if (g_isBufferObjectsSupported && (g_hasContext))
        OGLCHECK(glDeleteBuffersFn(1, &m_id));
    }

    bool BufferObject::isLocked() const
    {
      return m_isLocked;
    }

    void * BufferObject::data()
    {
      ASSERT(m_isLocked, ("BufferObject is not locked"));
      return m_gpuData;
    }

    void * BufferObject::lock()
    {
      ASSERT(!m_isLocked, ());
      m_isLocked = true;

      if (g_isMapBufferSupported)
      {
        m_isUsingMapBuffer = true;
        Bind();
        OGLCHECK(glBufferDataFn(m_target, m_size, 0, GL_DYNAMIC_DRAW));
        if (graphics::gl::g_hasContext)
          m_gpuData = glMapBufferFn(m_target, GL_WRITE_ONLY_MWM);
        else
        {
          m_gpuData = 0;
          LOG(LDEBUG, ("no OGL context. skipping OGL call"));
        }
        OGLCHECKAFTER;

        if (m_gpuData != 0)
          return m_gpuData;
      }

      m_isUsingMapBuffer = false;
      m_sharedBuffer = SharedBufferManager::instance().reserveSharedBuffer(m_size);

      m_gpuData = &m_sharedBuffer->at(0);
      return m_gpuData;
    }

    void BufferObject::unlock()
    {
      ASSERT(m_isLocked, ());
      m_isLocked = false;

      if (g_isBufferObjectsSupported)
      {
        ASSERT(m_gpuData != 0, ("BufferObject is not locked"));

        Bind();

        if (g_isMapBufferSupported && m_isUsingMapBuffer)
        {
          if (graphics::gl::g_hasContext)
          {
            if (glUnmapBufferFn(m_target) == GL_FALSE)
              LOG(LDEBUG/*LWARNING*/, ("glUnmapBuffer returned GL_FALSE!"));
            OGLCHECKAFTER;
          }
          else
            LOG(LDEBUG, ("no OGL context. skipping OGL call."));
        }
        else
        {
          m_isUsingMapBuffer = false;
          OGLCHECK(glBufferDataFn(m_target, m_size, m_gpuData, GL_DYNAMIC_DRAW));
          SharedBufferManager::instance().freeSharedBuffer(m_size, m_sharedBuffer);
          m_sharedBuffer.reset();
        }

        m_gpuData = 0;
      }
      Unbind();
    }

    void BufferObject::discard()
    {
      if (!g_isBufferObjectsSupported)
      {
        if (m_sharedBuffer)
        {
          SharedBufferManager::instance().freeSharedBuffer(m_size, m_sharedBuffer);
          m_sharedBuffer.reset();
          m_gpuData = 0;
        }
      }
    }

    void * BufferObject::glPtr()
    {
      if (!g_isBufferObjectsSupported)
        return m_gpuData;
      else
        return 0;
    }

    void BufferObject::makeCurrent(Binder & binder)
    {
/*#ifndef OMIM_OS_ANDROID
      if (m_id != current())
#endif*/
      binder.Reset(this);
    }

    void BufferObject::Bind()
    {
      if (!g_isBufferObjectsSupported || m_bound)
        return;

      OGLCHECK(glBindBufferFn(m_target, m_id));
      m_bound = true;
    }

    void BufferObject::Unbind()
    {
      if (!m_bound)
        return;

      m_bound = false;
#ifdef OMIM_OS_IPHONE
      OGLCHECK(glBindBufferFn(m_target, 0));
#else
      UNUSED_VALUE(m_target);
#endif
    }
  }
}
