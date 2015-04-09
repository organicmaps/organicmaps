#pragma once

#include "std/shared_ptr.hpp"
#include "graphics/render_target.hpp"
#include "graphics/data_formats.hpp"

namespace graphics
{
  namespace gl
  {
    class RenderBuffer : public RenderTarget
    {
    private:

      mutable unsigned int m_id;
      bool m_isDepthBuffer;

      size_t m_width;
      size_t m_height;

    public:

      // Create depth buffer
      RenderBuffer(size_t width, size_t height, bool isDepthBuffer = false, bool isRgba4 = false);
      ~RenderBuffer();

      unsigned int id() const;
      void makeCurrent() const;

      void attachToFrameBuffer();
      void detachFromFrameBuffer();

      bool isDepthBuffer() const;

      static int current();

      unsigned width() const;
      unsigned height() const;
    };
  }
}
