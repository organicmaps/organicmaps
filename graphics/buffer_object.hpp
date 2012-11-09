#pragma once

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace graphics
{
  namespace gl
  {
    class BufferObject
    {
    private:

      unsigned m_target;
      unsigned int m_id;
      unsigned int m_size;
      void * m_gpuData;
      bool m_isLocked;
      bool m_isUsingMapBuffer;
      shared_ptr<vector<unsigned char> > m_sharedBuffer;

    public:

      BufferObject(unsigned target);
      BufferObject(size_t size, unsigned target);
      ~BufferObject();

      void resize(size_t size);
      size_t size() const;

      void makeCurrent();

      void * lock();
      void unlock();
      void discard();

      void * glPtr();
      void * data();
      bool isLocked() const;
    };
  }
}
