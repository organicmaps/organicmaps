#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"

#include "render_state.hpp"
#include "renderbuffer.hpp"
#include "info_layer.hpp"

namespace yg
{
  namespace gl
  {
    RenderState::RenderState()
    : m_actualInfoLayer(new yg::InfoLayer()),
      m_isEmptyModelActual(false),
      m_currentInfoLayer(new yg::InfoLayer()),
      m_isEmptyModelCurrent(false),
      m_isResized(false),
      m_doRepaintAll(false),
      m_mutex(new threads::Mutex())
    {}

    bool RenderState::isPanning() const
    {
      return IsPanningAndRotate(m_actualScreen, m_currentScreen);
    }

    void RenderState::copyTo(RenderState &s) const
    {
      threads::MutexGuard guard(*m_mutex.get());
      s = *this;
    }

    void RenderState::addInvalidateFn(invalidateFn fn)
    {
      m_invalidateFns.push_back(fn);
    }

    void RenderState::invalidate()
    {
      for (list<invalidateFn>::const_iterator it = m_invalidateFns.begin(); it != m_invalidateFns.end(); ++it)
        (*it)();
    }

    void RenderState::onSize(size_t w, size_t h)
    {
      threads::MutexGuard guard(*m_mutex.get());
      if ((m_surfaceWidth != w) || (m_surfaceHeight != h))
      {
        m_isResized = true;

        m_surfaceWidth = w;
        m_surfaceHeight = h;

        double const log2 = log(2.0);

        m_textureWidth = static_cast<uint32_t>(pow(2, ceil(log(double(w)) / log2)));
        m_textureHeight = static_cast<uint32_t>(pow(2, ceil(log(double(h)) / log2)));
      }
    }

    m2::PointU const RenderState::coordSystemShift(bool doLock) const
    {
      if (doLock)
        m_mutex->Lock();
      m2::PointU const res((m_textureWidth - m_surfaceWidth) / 2,
                     (m_textureHeight - m_surfaceHeight) / 2);
      if (doLock)
        m_mutex->Unlock();
      return res;
    }
  }
}
