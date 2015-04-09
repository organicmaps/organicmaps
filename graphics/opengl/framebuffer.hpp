#pragma once

#include "std/shared_ptr.hpp"

namespace graphics
{
  class RenderTarget;

  namespace gl
  {
    class BaseTexture;
    class RenderBuffer;

    class FrameBuffer
    {
    private:

      unsigned int m_id;

      shared_ptr<RenderTarget> m_renderTarget;
      shared_ptr<RenderBuffer> m_depthBuffer;

      unsigned m_width;
      unsigned m_height;

    public:

      FrameBuffer(bool defaultFB = false);
      ~FrameBuffer();

      int id() const;

      void setRenderTarget(shared_ptr<RenderTarget> const & renderTarget);
      shared_ptr<RenderTarget> const & renderTarget() const;
      void resetRenderTarget();

      void setDepthBuffer(shared_ptr<RenderBuffer> const & depthBuffer);
      shared_ptr<RenderBuffer> const & depthBuffer() const;
      void resetDepthBuffer();

      void makeCurrent();

      void onSize(unsigned width, unsigned height);

      unsigned width() const;
      unsigned height() const;

      static unsigned current();

      void checkStatus();
    };
  }
}
