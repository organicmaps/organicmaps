#pragma once

#include "color.hpp"

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

      struct BaseState
      {
        virtual ~BaseState();
        virtual void apply(BaseState const * prev) = 0;
      };

      struct State : BaseState
      {
        shared_ptr<FrameBuffer> m_frameBuffer;
        shared_ptr<RenderTarget> m_renderTarget;
        shared_ptr<RenderTarget> m_depthBuffer;
        shared_ptr<ResourceManager> m_resourceManager;

        void apply(BaseState const * prev);
      };

      struct Command
      {
      private:
        bool m_isDebugging;
      public:

        bool isDebugging() const;

        Command();

        virtual ~Command();
        virtual void perform() = 0;

        friend class Renderer;
      };

      struct Packet
      {
        shared_ptr<BaseState> m_state;
        shared_ptr<Command> m_command;
        Packet();
        Packet(shared_ptr<BaseState> const & state,
               shared_ptr<Command> const & command);
      };

      struct ClearCommand : Command
      {
        yg::Color m_color;
        bool m_clearRT;
        float m_depth;
        bool m_clearDepth;

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
        ThreadedList<Packet> * m_renderQueue;
        Params();
      };

    private:

      shared_ptr<FrameBuffer> m_frameBuffer;
      shared_ptr<RenderTarget> m_renderTarget;
      shared_ptr<RenderTarget> m_depthBuffer;
      shared_ptr<ResourceManager> m_resourceManager;

      ThreadedList<Packet> * m_renderQueue;

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

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);
      shared_ptr<RenderTarget> const & renderTarget() const;

      void setDepthBuffer(shared_ptr<RenderTarget> const & rt);
      shared_ptr<RenderTarget> const & depthBuffer() const;

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

      virtual shared_ptr<BaseState> const createState() const;

      virtual void getState(BaseState * state);

      void processCommand(shared_ptr<Command> const & command);
      ThreadedList<Packet> * renderQueue();
    };
  }
}
