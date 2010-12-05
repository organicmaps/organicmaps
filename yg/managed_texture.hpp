#pragma once

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "base_texture.hpp"

namespace yg
{
  namespace gl
  {
    class ManagedTexture : public BaseTexture
    {
    private:
      /// do we have a data to be updated on unlock?
      bool m_isDirty;
      /// cumulative dirty rect.
      m2::RectU m_dirtyRect;

    protected:

      /// is the texture locked
      bool m_isLocked;

      virtual void updateDirty(m2::RectU const & r) = 0;
      virtual void upload() = 0;

    public:

      ManagedTexture(m2::PointU const & size);
      ManagedTexture(unsigned width, unsigned height);

      void lock();
      void addDirtyRect(m2::RectU const & r);
      void unlock();
    };
  }
}
