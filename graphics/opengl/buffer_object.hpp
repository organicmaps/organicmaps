#pragma once

#include "../../std/vector.hpp"
#include "../../std/shared_ptr.hpp"

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

    public:
      class Binder;
      void makeCurrent(shared_ptr<Binder> & bindBuf);

      class Binder
      {
        friend void BufferObject::makeCurrent(shared_ptr<Binder> & bindBuf);
        unsigned int const m_target;

        Binder(unsigned int target, unsigned int id);
        Binder(const Binder &) = delete;
        Binder const & operator=(const Binder &) = delete;
      public:
        ~Binder();
      };

      BufferObject(unsigned target);
      BufferObject(size_t size, unsigned target);
      ~BufferObject();

      void resize(size_t size);
      size_t size() const;



      void * lock(shared_ptr<Binder> & bindBuf);
      void unlock(shared_ptr<Binder> & bindBuf);
      void discard();

      void * glPtr();
      void * data();
      bool isLocked() const;
    };
  }
}
