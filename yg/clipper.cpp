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

    Clipper::EnableClipRect::EnableClipRect(bool flag)
      : m_flag(flag)
    {}

    void Clipper::EnableClipRect::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing EnableClipRect command"));

      if (m_flag)
      {
        if (isDebugging())
          LOG(LINFO, ("enabling scissor test"));
        OGLCHECK(glEnableFn(GL_SCISSOR_TEST));
      }
      else
      {
        if (isDebugging())
        {
          LOG(LINFO, ("disabling scissor test"));
          OGLCHECK(glDisableFn(GL_SCISSOR_TEST));
        }
      }
    }

    void Clipper::enableClipRect(bool flag)
    {
      m_isClippingEnabled = flag;

      processCommand(make_shared_ptr(new EnableClipRect(flag)));
    }

    bool Clipper::clipRectEnabled() const
    {
      return m_isClippingEnabled;
    }

    Clipper::SetClipRect::SetClipRect(m2::RectI const & rect)
      : m_rect(rect)
    {}

    void Clipper::SetClipRect::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing SetClipRect command"));

      OGLCHECK(glScissor(m_rect.minX(), m_rect.minY(), m_rect.SizeX(), m_rect.SizeY()));
    }

    void Clipper::setClipRect(m2::RectI const & rect)
    {
      m_clipRect = rect;
      if (!m_clipRect.Intersect(m2::RectI(0, 0, width(), height())))
        m_clipRect = m2::RectI(0, 0, 0, 0);

      ASSERT ( m_clipRect.IsValid(), (m_clipRect) );

      processCommand(make_shared_ptr(new SetClipRect(m_clipRect)));
    }

    m2::RectI const & Clipper::clipRect() const
    {
      return m_clipRect;
    }
  }
}
