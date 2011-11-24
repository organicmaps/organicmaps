#pragma once

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class VertexBuffer
    {
    private:

      unsigned int m_id;
      unsigned int m_size;
      void * m_gpuData;

      shared_ptr<vector<unsigned char> > m_sharedBuffer;

      /// using VA instead of buffer objects on some old GPU's
      bool m_useVA;
      bool m_isLocked;

    public:

      VertexBuffer(bool useVA);
      VertexBuffer(size_t size, bool useVA);
      ~VertexBuffer();

      void resize(size_t size);
      size_t size() const;

      void makeCurrent();
      void * lock();
      void unlock();
      void discard();
      void * glPtr();
      void * data();
      bool isLocked() const;

      static unsigned current();
    };
  }
}
