#pragma once

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

    public:

      VertexBuffer();
      VertexBuffer(size_t size);
      ~VertexBuffer();

      void resize(size_t size);
      size_t size() const;

      void makeCurrent();
      void * lock();
      void unlock();

      static unsigned current();
      static void pushCurrent();
      static void popCurrent();
    };
  }
}
