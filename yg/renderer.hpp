#pragma once

#include "color.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  class ResourceManager;

  namespace gl
  {
    class FrameBuffer;
    class RenderBuffer;
    class BaseTexture;
    class RenderTarget;

    class Renderer
    {
    public:
      virtual ~Renderer() {}

      struct Params
      {
        shared_ptr<ResourceManager> m_resourceManager;
        shared_ptr<FrameBuffer> m_frameBuffer;
        bool m_isDebugging;
        Params();
      };

    private:

      shared_ptr<ResourceManager> m_resourceManager;

      shared_ptr<FrameBuffer> m_frameBuffer;
      shared_ptr<BaseTexture> m_renderTarget;
      shared_ptr<RenderBuffer> m_depthBuffer;

      bool m_isDebugging;

      bool m_isRendering;

      unsigned int m_width;
      unsigned int m_height;

    public:

      Renderer(Params const & params = Params());

      void beginFrame();
      void endFrame();

      bool isRendering() const;

      shared_ptr<ResourceManager> const & resourceManager() const;

      shared_ptr<FrameBuffer> const & frameBuffer() const;
//      void setFrameBuffer(shared_ptr<FrameBuffer> const & fb);

      shared_ptr<FrameBuffer> const & multiSampledFrameBuffer() const;

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);
      shared_ptr<RenderTarget> const & renderTarget() const;

      /// @param clearRT - should we clear the renderTarget data (visible pixels)?
      /// @param clearDepth - should we clear depthBuffer data?
      /// @warning this function respects the clipping rect set and enabled(!)
      ///          by the setClipRect/enableClipRect. Whether the clipping is
      ///          not enabled -  the entire currently bound render surface is used.
      void clear(yg::Color const & c = yg::Color(187, 187, 187, 255), bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      void onSize(unsigned width, unsigned height);

      unsigned int width() const;
      unsigned int height() const;

      void finish();

      bool isDebugging() const;
    };
  }
}
