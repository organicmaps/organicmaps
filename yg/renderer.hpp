#pragma once

#include "../std/shared_ptr.hpp"
#include "texture.hpp"
#include "color.hpp"

namespace yg
{
  namespace gl
  {
    class FrameBuffer;
    class RenderBuffer;

    class Renderer
    {
    private:

      shared_ptr<FrameBuffer> m_frameBuffer;
      shared_ptr<BaseTexture> m_renderTarget;
      shared_ptr<RenderBuffer> m_depthBuffer;

      shared_ptr<FrameBuffer> m_multiSampledFrameBuffer;
      shared_ptr<RenderBuffer> m_multiSampledRenderTarget;
      shared_ptr<RenderBuffer> m_multiSampledDepthBuffer;

      bool m_isMultiSampled;

      bool m_isRendering;

      unsigned int m_width;
      unsigned int m_height;

    public:

      Renderer();

      void beginFrame();
      void endFrame();

      bool isRendering() const;

      void setIsMultiSampled(bool isMultiSampled);
      bool isMultiSampled() const;

      shared_ptr<FrameBuffer> const & frameBuffer() const;
      void setFrameBuffer(shared_ptr<FrameBuffer> const & fb);

      shared_ptr<FrameBuffer> const & multiSampledFrameBuffer() const;

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);
      shared_ptr<RenderTarget> const & renderTarget() const;

      void updateFrameBuffer();

      /// @param clearRT - should we clear the renderTarget data (visible pixels)?
      /// @param clearDepth - should we clear depthBuffer data?
      /// @warning this function respects the clipping rect set and enabled(!)
      ///          by the setClipRect/enableClipRect. Whether the clipping is
      ///          not enabled -  the entire currently bound render surface is used.
      void clear(yg::Color const & c = yg::Color(192, 192, 192, 255), bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      void onSize(unsigned width, unsigned height);

      unsigned int width() const;
      unsigned int height() const;
    };
  }
}
