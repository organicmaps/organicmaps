#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

#include "graphics/opengl/base_texture.hpp"

namespace graphics
{
  namespace gl
  {
    class ManagedTexture : public BaseTexture
    {
    private:

      /// size of the allocated shared buffers
      size_t m_imageSize;

    protected:

      /// is the texture locked
      bool m_isLocked;

      virtual void upload(void * data) = 0;
      virtual void upload(void * data, m2::RectU const & r) = 0;

      /// system memory buffer for the purpose of correct working of glTexSubImage2D.
      /// in OpenGL ES 1.1 there are no way to specify GL_UNPACK_ROW_LENGTH so
      /// the data supplied to glTexSubImage2D should be continous.
      shared_ptr<vector<unsigned char> > m_auxData;

    public:

      ManagedTexture(m2::PointU const & size, size_t pixelSize);
      ManagedTexture(unsigned width, unsigned height, size_t pixelSize);

      void lock();
      void unlock();
      void * auxData();
    };
  }
}
