#include "std/bind.hpp"
#include "base/logging.hpp"

#include "graphics/opengl/clipper.hpp"
#include "graphics/opengl/opengl.hpp"

namespace graphics
{
  namespace gl
  {
    Clipper::Clipper(base_t::Params const & params)
      : base_t(params),
      m_isClippingEnabled(false)
    {}

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

    Clipper::EnableClipRect::EnableClipRect(bool flag)
      : m_flag(flag)
    {}

    void Clipper::EnableClipRect::perform()
    {
      if (m_flag)
        OGLCHECK(glEnable(GL_SCISSOR_TEST));
      else
        OGLCHECK(glDisable(GL_SCISSOR_TEST));
    }

    void Clipper::enableClipRect(bool flag)
    {
      m_isClippingEnabled = flag;

      processCommand(make_shared<EnableClipRect>(flag));
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
      OGLCHECK(glScissor(m_rect.minX(), m_rect.minY(), m_rect.SizeX(), m_rect.SizeY()));
    }

    void Clipper::setClipRect(m2::RectI const & rect)
    {
      m_clipRect = rect;
      if (!m_clipRect.Intersect(m2::RectI(0, 0, width(), height())))
        m_clipRect = m2::RectI(0, 0, 0, 0);

      ASSERT ( m_clipRect.IsValid(), (m_clipRect) );

      processCommand(make_shared<SetClipRect>(m_clipRect));
    }

    m2::RectI const & Clipper::clipRect() const
    {
      return m_clipRect;
    }
  }
}
