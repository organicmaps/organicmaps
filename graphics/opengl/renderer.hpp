#pragma once

#include "graphics/color.hpp"
#include "graphics/packets_queue.hpp"
#include "graphics/resource_manager.hpp"

#include "base/threaded_list.hpp"
#include "base/commands_queue.hpp"
#include "std/function.hpp"
#include "std/shared_ptr.hpp"
#include "geometry/rect2d.hpp"

namespace graphics
{
  class ResourceManager;
  class RenderTarget;
  class RenderContext;

  namespace gl
  {
    class FrameBuffer;
    class RenderBuffer;
    class BaseTexture;
    class Program;

    class Renderer
    {
    public:

      struct ClearCommand : Command
      {
        graphics::Color m_color;
        bool m_clearRT;
        float m_depth;
        bool m_clearDepth;

        ClearCommand(graphics::Color const & color,
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

      struct ChangeMatrix : Command
      {
        EMatrix m_matrixType;
        math::Matrix<float, 4, 4> m_matrix;

        ChangeMatrix(EMatrix mt, math::Matrix<float, 4, 4> const & m);
        void perform();
      };

      struct DiscardFramebuffer : Command
      {
        bool m_doDiscardColor;
        bool m_doDiscardDepth;
        DiscardFramebuffer(bool doDiscardColor, bool doDiscardDepth);
        void perform();
      };

      struct CopyFramebufferToImage : Command
      {
        shared_ptr<BaseTexture> m_target;

        CopyFramebufferToImage(shared_ptr<BaseTexture> target);

        void perform();
      };

      virtual ~Renderer();

      struct Params
      {
        shared_ptr<RenderContext> m_renderContext;
        shared_ptr<ResourceManager> m_resourceManager;
        shared_ptr<FrameBuffer> m_frameBuffer;
        bool m_isDebugging;
        PacketsQueue * m_renderQueue;
        int m_threadSlot;
        Params();
      };

    private:

      shared_ptr<FrameBuffer> m_frameBuffer;
      shared_ptr<RenderTarget> m_renderTarget;
      shared_ptr<RenderBuffer> m_depthBuffer;
      shared_ptr<ResourceManager> m_resourceManager;

      PacketsQueue * m_renderQueue;

      bool m_isDebugging;

      bool m_isRendering;

      unsigned int m_width;
      unsigned int m_height;

      core::CommandsQueue::Environment const * m_env;

      int m_threadSlot;

      shared_ptr<RenderContext> m_renderContext;

    public:

      static const graphics::Color s_bgColor;

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

      void discardFramebuffer(bool doDiscardColor, bool doDiscardDepth);
      void copyFramebufferToImage(shared_ptr<BaseTexture> target);

      /// @param clearRT - should we clear the renderTarget data (visible pixels)?
      /// @param clearDepth - should we clear depthBuffer data?
      /// @warning this function respects the clipping rect set and enabled(!)
      ///          by the setClipRect/enableClipRect. Whether the clipping is
      ///          not enabled -  the entire currently bound render surface is used.
      void clear(graphics::Color const & c, bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      void onSize(unsigned width, unsigned height);

      unsigned int width() const;
      unsigned int height() const;

      bool isDebugging() const;

      void processCommand(shared_ptr<Command> const & command, Packet::EType type = Packet::ECommand, bool doForce = false);
      PacketsQueue * renderQueue();

      void addCheckPoint();
      void addFramePoint();

      void completeCommands();

      void setEnvironment(core::CommandsQueue::Environment const * env);
      bool isCancelled() const;

      void setProgram(shared_ptr<Program> const & prg);
      shared_ptr<Program> const & program() const;

      RenderContext * renderContext() const;

      int threadSlot() const;
    };
  }
}
