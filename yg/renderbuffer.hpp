#pragma once

#include "../std/shared_ptr.hpp"
#include "render_target.hpp"

namespace yg
{
  namespace gl
  {
    class RenderBuffer : public RenderTarget
    {
    private:

      unsigned int m_id;
      bool m_isDepthBuffer;
      bool m_isMultiSampled;

      size_t m_width;
      size_t m_height;

    public:

      RenderBuffer(size_t width, size_t height, bool isDepthBuffer = false, bool isMultiSampled = false);
      ~RenderBuffer();

      unsigned int id() const;
      void makeCurrent();

      void attachToFrameBuffer();

      bool isDepthBuffer() const;
      bool isMultiSampled() const;

      static int current();

      unsigned width() const;
      unsigned height() const;

      static void pushCurrent();
      static void popCurrent();
    };
  }
}
