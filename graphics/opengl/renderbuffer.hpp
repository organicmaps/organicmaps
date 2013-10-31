#pragma once

#include "../../std/shared_ptr.hpp"
#include "../render_target.hpp"
#include "../data_formats.hpp"

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
      RenderBuffer(size_t width, size_t height);
      // Create color bufer
      RenderBuffer(size_t width, size_t height, graphics::DataFormat format);
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
