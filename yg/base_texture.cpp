#include "../base/SRC_FIRST.hpp"

#include "internal/opengl.hpp"
#include "base_texture.hpp"
#include "utils.hpp"


namespace yg
{
  namespace gl
  {
    void BaseTexture::checkID() const
    {
      if (!m_hasID)
      {
        m_hasID = true;
        init();
      }
    }

    void BaseTexture::init() const
    {
      OGLCHECK(glGenTextures(1, &m_id));

      OGLCHECK(glBindTexture(GL_TEXTURE_2D, m_id));

      OGLCHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
      OGLCHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
      OGLCHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    }

    BaseTexture::BaseTexture(m2::PointU const & size)
      : m_id(0), m_hasID(false), m_width(size.x), m_height(size.y)
    {
//      init();
    }

    BaseTexture::BaseTexture(unsigned width, unsigned height)
      : m_id(0), m_hasID(false), m_width(width), m_height(height)
    {
//      init();
    }

    BaseTexture::~BaseTexture()
    {
      if ((m_hasID) && (g_doDeleteOnDestroy))
        OGLCHECK(glDeleteTextures(1, &m_id));
    }

    unsigned BaseTexture::width() const
    {
      return m_width;
    }

    unsigned BaseTexture::height() const
    {
      return m_height;
    }

    void BaseTexture::attachToFrameBuffer()
    {
      checkID();
#ifdef OMIM_GL_ES
      OGLCHECK(glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, id(), 0));
#else
      OGLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, id(), 0));
#endif
      utils::setupCoordinates(width(), height(), false);
    }

    unsigned BaseTexture::current()
    {
      int id;
      OGLCHECK(glGetIntegerv(GL_TEXTURE_BINDING_2D, &id));
      return id;
    }

    void BaseTexture::makeCurrent() const
    {
      checkID();
#ifndef OMIM_OS_ANDROID
      if (current() != m_id)
#endif
      OGLCHECK(glBindTexture(GL_TEXTURE_2D, m_id));
    }

    unsigned BaseTexture::id() const
    {
      checkID();
      return m_id;
    }

    m2::PointF const BaseTexture::mapPixel(m2::PointF const & p) const
    {
      return m2::PointF(p.x / (float) width() /*+ 0.0 / width()*/,
                        p.y / (float) height() /*+ 0.0 / height()*/);
    }

    void BaseTexture::mapPixel(float & x, float & y) const
    {
      x = x / width();
      y = y / height();
    }

    m2::RectF const BaseTexture::mapRect(m2::RectF const & r) const
    {
      m2::PointF pt1(r.minX(), r.minY());
      m2::PointF pt2(r.maxX(), r.maxY());

      pt1 = mapPixel(pt1);
      pt2 = mapPixel(pt2);

      return m2::RectF(pt1.x, pt1.y, pt2.x, pt2.y);
    }
  }
}
