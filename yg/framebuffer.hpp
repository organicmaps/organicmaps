#pragma once

#include "../std/shared_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class BaseTexture;
    class RenderBuffer;
    class RenderTarget;

    class FrameBuffer
    {
    private:

      unsigned int m_id;

      shared_ptr<RenderTarget> m_renderTarget;
      shared_ptr<RenderTarget> m_depthBuffer;

      unsigned m_width;
      unsigned m_height;

    public:

      FrameBuffer(bool defaultFB = false);
      ~FrameBuffer();

      int id() const;

      void setRenderTarget(shared_ptr<RenderTarget> const & renderTarget);
      shared_ptr<RenderTarget> const & renderTarget() const;
      void resetRenderTarget();

      void setDepthBuffer(shared_ptr<RenderTarget> const & depthBuffer);
      shared_ptr<RenderTarget> const & depthBuffer() const;
      void resetDepthBuffer();

      void makeCurrent();

      void onSize(unsigned width, unsigned height);

      unsigned width() const;
      unsigned height() const;

      static unsigned current();
      static void pushCurrent();
      static void popCurrent();

      void checkStatus();
    };
  }
}
