#include "../base/SRC_FIRST.hpp"

#include "internal/opengl.hpp"
#include "managed_texture.hpp"

namespace yg
{
  namespace gl
  {
    ManagedTexture::ManagedTexture(unsigned width, unsigned height)
      : BaseTexture(width, height), m_isLocked(false)
    {}

    ManagedTexture::ManagedTexture(m2::PointU const & size)
      : BaseTexture(size.x, size.y), m_isLocked(false)
    {}

    void ManagedTexture::addDirtyRect(m2::RectU const & r)
    {
      if (m_isLocked)
      {
        if (!m_isDirty && (r.SizeX() != 0) && (r.SizeY() != 0))
        {
          m_dirtyRect = r;
          m_isDirty = true;
        }
        else
          m_dirtyRect.Add(r);
      }
    }

    void ManagedTexture::lock()
    {
      m_isLocked = true;
      m_isDirty = false;
    }

    void ManagedTexture::unlock()
    {
      m_isLocked = false;

      if (m_isDirty)
      {
        updateDirty(m_dirtyRect);
        m_isDirty = false;
      }
    }
  }
}
