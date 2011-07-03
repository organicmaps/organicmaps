#include "render_state_updater.hpp"
#include "render_state.hpp"
#include "framebuffer.hpp"
#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "info_layer.hpp"

#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    RenderStateUpdater::Params::Params()
      : m_doPeriodicalUpdate(false), m_updateInterval(0.0)
    {}

    RenderStateUpdater::RenderStateUpdater(Params const & params)
      : base_t(params),
      m_renderState(params.m_renderState),
      m_doPeriodicalUpdate(params.m_doPeriodicalUpdate),
      m_updateInterval(params.m_updateInterval)
    {
    }

    shared_ptr<RenderState> const & RenderStateUpdater::renderState() const
    {
      return m_renderState;
    }

    void RenderStateUpdater::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                          shared_ptr<VertexBuffer> const & vertices,
                                          shared_ptr<IndexBuffer> const & indices,
                                          size_t indicesCount)
    {
      base_t::drawGeometry(texture, vertices, indices, indicesCount);
      m_indicesCount += indicesCount;
      if (m_doPeriodicalUpdate
       && m_renderState
       && (m_indicesCount > 20000)
       && (m_updateTimer.ElapsedSeconds() > m_updateInterval))
      {
        updateActualTarget();
        m_indicesCount %= 20000;
        m_updateTimer.Reset();
      }
    }

    void RenderStateUpdater::updateActualTarget()
    {}

    void RenderStateUpdater::beginFrame()
    {
      base_t::beginFrame();
      m_indicesCount = 0;
      m_updateTimer.Reset();
    }

    void RenderStateUpdater::setClipRect(m2::RectI const & rect)
    {
      if ((m_renderState) && (m_indicesCount))
      {
        updateActualTarget();
        m_indicesCount = 0;
        m_updateTimer.Reset();
      }

      base_t::setClipRect(rect);
    }

    void RenderStateUpdater::endFrame()
    {
      if (m_renderState)
        updateActualTarget();
      m_indicesCount = 0;
      m_updateTimer.Reset();
      base_t::endFrame();
    }
  }
}
