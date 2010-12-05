#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"

#include "render_state.hpp"
#include "renderbuffer.hpp"

namespace yg
{
  namespace gl
  {
    RenderState::RenderState()
      :  m_isResized(false),
      m_doRepaintAll(false),
      m_mutex(new threads::Mutex())
    {}

    bool RenderState::isPanning() const
    {
      return IsPanning(m_actualScreen, m_currentScreen);
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

        m_textureWidth = pow(2, ceil(log(double(w)) / log2));
        m_textureHeight =  pow(2, ceil(log(double(h)) / log2));


/*        int maxTextureSize;
//        OGLCHECK(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize));
        maxTextureSize = 2048;

        m_textureWidth = min(m_textureWidth * 2, (unsigned) maxTextureSize);
        m_textureHeight = min(m_textureHeight * 2, (unsigned) maxTextureSize);

//        LOG(LINFO, ("TextureSize :", m_textureWidth, m_textureHeight));
*/
      }
    }
  }
}
