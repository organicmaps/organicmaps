#pragma once

namespace yg
{
  namespace gl
  {
    class IndexBuffer
    {
    private:

      unsigned int m_id;
      unsigned int m_size;
      void * m_gpuData;
      bool m_useVA;
      bool m_isLocked;

    public:

      IndexBuffer(bool useVA);
      IndexBuffer(size_t size, bool useVA);
      ~IndexBuffer();

      void resize(size_t size);
      size_t size() const;

      void makeCurrent();
      void * lock();
      void unlock();

      void * glPtr();
      void * data();
      bool isLocked() const;

      static int current();
    };
  }
}
