#include "../base/SRC_FIRST.hpp"
#include "clipper.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    Clipper::Clipper()
      : m_isClippingEnabled(false)
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

    void Clipper::enableClipRect(bool flag)
    {
      m_isClippingEnabled = flag;
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

      OGLCHECK(glScissor(m_clipRect.minX(), m_clipRect.minY(), m_clipRect.SizeX(), m_clipRect.SizeY()));
    }

    m2::RectI const & Clipper::clipRect() const
    {
      return m_clipRect;
    }
  }
}
