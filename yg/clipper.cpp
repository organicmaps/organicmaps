#include "../base/SRC_FIRST.hpp"
#include "clipper.hpp"
#include "internal/opengl.hpp"
#include "../std/bind.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    Clipper::Clipper(base_t::Params const & params)
      : base_t(params),
      m_isClippingEnabled(false)
    {}

    void Clipper::State::apply(BaseState const * prev)
    {
      base_t::State::apply(prev);

      State const * state = static_cast<State const *>(prev);

      if (state->m_isClippingEnabled != m_isClippingEnabled)
      {
        if (m_isClippingEnabled)
        {
          if (m_isDebugging)
            LOG(LINFO, ("enabling scissors"));
          OGLCHECK(glEnable(GL_SCISSOR_TEST));
        }
        else
        {
          if (m_isDebugging)
            LOG(LINFO, ("disabling scissors"));
          OGLCHECK(glDisable(GL_SCISSOR_TEST));
        }
      }

      if (state->m_clipRect != m_clipRect)
      {
        if (m_isDebugging)
          LOG(LINFO, ("scissor(", m_clipRect.minX(), m_clipRect.minY(), m_clipRect.maxX(), m_clipRect.maxY(), ")"));
        OGLCHECK(glScissor(m_clipRect.minX(), m_clipRect.minY(), m_clipRect.SizeX(), m_clipRect.SizeY()));
      }
    }

    void Clipper::beginFrame()
    {
      base_t::beginFrame();

      enableClipRect(m_isClippingEnabled);
      setClipRect(m_clipRect);
    }

    void Clipper::endFrame()
    {
      /// clipper code goes here
      base_t::endFrame();
    }

    void Clipper::enableClipRect(bool flag)
    {
      m_isClippingEnabled = flag;

      if (renderQueue())
        return;

      if (m_isClippingEnabled)
        OGLCHECK(glEnable(GL_SCISSOR_TEST));
      else
        OGLCHECK(glDisable(GL_SCISSOR_TEST));
    }

    bool Clipper::clipRectEnabled() const
    {
      return m_isClippingEnabled;
    }

    void Clipper::setClipRect(m2::RectI const & rect)
    {
      m_clipRect = rect;
      if (!m_clipRect.Intersect(m2::RectI(0, 0, width(), height())))
        m_clipRect = m2::RectI(0, 0, 0, 0);

      if (renderQueue())
        return;

      ASSERT ( m_clipRect.IsValid(), (m_clipRect) );
      OGLCHECK(glScissor(m_clipRect.minX(), m_clipRect.minY(), m_clipRect.SizeX(), m_clipRect.SizeY()));
    }

    m2::RectI const & Clipper::clipRect() const
    {
      return m_clipRect;
    }

    shared_ptr<BaseState> const Clipper::createState() const
    {
      return shared_ptr<BaseState>(new State());
    }

    void Clipper::getState(BaseState * s)
    {
      State * state = static_cast<State *>(s);
      base_t::getState(s);

      state->m_clipRect = m_clipRect;
      state->m_isClippingEnabled = m_isClippingEnabled;
    }
  }
}
