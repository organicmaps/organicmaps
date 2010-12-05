#pragma once

#include "rendercontext.hpp"
#include "render_state.hpp"
#include "render_command.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/unordered_map.hpp"
#include "../std/list.hpp"
#include "../base/thread.hpp"
#include "../base/mutex.hpp"
#include "../base/condition.hpp"

class ScreenBase;

namespace yg
{
  namespace gl
  {
    class BaseTexture;
    class RenderContext;
    class FrameBuffer;
    class RenderBuffer;
    class RenderState;

    class ThreadRenderer
    {
    private:

      shared_ptr<RenderContext> m_renderContext;
      shared_ptr<RenderState> m_renderState;
      shared_ptr<Blitter> m_blitter;

      RenderState m_renderStateCopy;

      /// Current states
      shared_ptr<BaseTexture> m_texture;
      m2::RectI m_viewport;

    public:

      ThreadRenderer();

      void init(shared_ptr<RenderContext> const & renderContext,
                shared_ptr<RenderState> const & renderState);

      void setupStates();

      void prepareBackBuffer();

      void beginFrame();
      void endFrame();

      void setCurrentScreen(ScreenBase const & currentScreen);

      void processResize();

      void setViewport(m2::RectI const & viewport);

      void drawGeometry(shared_ptr<BaseTexture> const & texture,
                        shared_ptr<VertexBuffer> const & vertices,
                        shared_ptr<IndexBuffer> const & indices,
                        size_t indicesCount);

      void updateActualTarget();

      void blitBackBuffer();

      void copyRenderState();
    };
  }
}
