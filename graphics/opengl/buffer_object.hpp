#pragma once

#include "base/macros.hpp"

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

namespace graphics
{
  namespace gl
  {
    class BufferObject
    {
    private:

      unsigned const m_target;
      unsigned int m_id;
      unsigned int m_size;
      void * m_gpuData;
      bool m_isLocked;
      bool m_isUsingMapBuffer;
      shared_ptr<vector<unsigned char> > m_sharedBuffer;
      bool m_bound;

    public:
      class Binder;
      /// Binds the buffer object to the current thread.
      /// The buffer object will be unbound from the current thread on calling the destructor of Binder in case of iOS.
      void makeCurrent(Binder & binder);

      /// This class is wrapper for binding and unbinding gl buffers.
      class Binder
      {
        friend void BufferObject::makeCurrent(Binder & binder);
        BufferObject * m_bufferObj;

        void Reset(BufferObject & bufferObj);

      public:
        Binder() : m_bufferObj(nullptr) {}
        ~Binder();
        DISALLOW_COPY_AND_MOVE(Binder);
      };

      BufferObject(unsigned target);
      BufferObject(size_t size, unsigned target);
      ~BufferObject();

      void resize(size_t size);
      size_t size() const;

      /// Multithreading notes for Bind and Unbind methods:
      /// 1. Method Unbind should be called from the same thread on which method Bind was called.
      /// 2. After Unbind() call the current instance can be used by other threads.
      /// Binds the buffer object to the current thread.
      void Bind();
      /// Unbinds the buffer object from the current thread only for iOS.
      void Unbind();
      bool IsBound() const { return m_bound; }

      /// This method locks the instance of a buffer object and returns a pointer to a GPU buffer.
      /// Notes. The buffer object will be bound to the current thread.
      /// It shall not be unbound until unlock is called.
      void * lock();
      /// Informs gl that the instance of a buffer object is released and unbind it.
      void unlock();
      void discard();

      void * glPtr();
      void * data();
      bool isLocked() const;
    };
  }
}
