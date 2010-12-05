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

    public:

      IndexBuffer();
      IndexBuffer(size_t size);
      ~IndexBuffer();

      void resize(size_t size);
      size_t size() const;

      void makeCurrent();
      void * lock();
      void unlock();

      static void pushCurrent();
      static void popCurrent();
      static int current();
    };
  }
}
