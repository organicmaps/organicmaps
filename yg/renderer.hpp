#pragma once

#include "color.hpp"
#include "packets_queue.hpp"

#include "../base/threaded_list.hpp"
#include "../std/function.hpp"
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

      struct ClearCommand : Command
      {
        yg::Color m_color;
        bool m_clearRT;
        float m_depth;
        bool m_clearDepth;

        ClearCommand(yg::Color const & color,
                     bool clearRT = true,
                     float depth = 1.0,
                     bool clearDepth = true);

        void perform();
      };

      struct ChangeFrameBuffer : Command
      {
        shared_ptr<FrameBuffer> m_frameBuffer;

        ChangeFrameBuffer(shared_ptr<FrameBuffer> const & fb);

        void perform();
      };

      struct UnbindRenderTarget : Command
      {
        shared_ptr<RenderTarget> m_renderTarget;
        UnbindRenderTarget(shared_ptr<RenderTarget> const & renderTarget);
        void perform();
      };

      struct FinishCommand : Command
      {
        void perform();
      };

      virtual ~Renderer();

      struct Params
      {
        shared_ptr<ResourceManager> m_resourceManager;
        shared_ptr<FrameBuffer> m_frameBuffer;
        bool m_isDebugging;
        bool m_doUnbindRT;
        bool m_isSynchronized;
        PacketsQueue * m_renderQueue;
        Params();
      };

    private:

      shared_ptr<FrameBuffer> m_frameBuffer;
      shared_ptr<RenderTarget> m_renderTarget;
      shared_ptr<RenderBuffer> m_depthBuffer;
      shared_ptr<ResourceManager> m_resourceManager;

      PacketsQueue * m_renderQueue;

      bool m_isDebugging;

      bool m_doUnbindRT;
      bool m_isSynchronized;

      bool m_isRendering;

      unsigned int m_width;
      unsigned int m_height;

    public:

      static const yg::Color s_bgColor;

      Renderer(Params const & params = Params());

      void beginFrame();
      void endFrame();

      bool isRendering() const;

      shared_ptr<ResourceManager> const & resourceManager() const;

      shared_ptr<FrameBuffer> const & frameBuffer() const;

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);
      shared_ptr<RenderTarget> const & renderTarget() const;
      void resetRenderTarget();

      void setDepthBuffer(shared_ptr<RenderBuffer> const & rt);
      shared_ptr<RenderBuffer> const & depthBuffer() const;
      void resetDepthBuffer();

      /// @param clearRT - should we clear the renderTarget data (visible pixels)?
      /// @param clearDepth - should we clear depthBuffer data?
      /// @warning this function respects the clipping rect set and enabled(!)
      ///          by the setClipRect/enableClipRect. Whether the clipping is
      ///          not enabled -  the entire currently bound render surface is used.
      void clear(yg::Color const & c, bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      void onSize(unsigned width, unsigned height);

      unsigned int width() const;
      unsigned int height() const;

      void finish();

      bool isDebugging() const;

      void processCommand(shared_ptr<Command> const & command, Packet::EType type = Packet::ECommand);
      PacketsQueue * renderQueue();

      /// insert empty packet into glQueue to mark the frame boundary
      void markFrameBoundary();
      void completeCommands();
    };
  }
}
