#pragma once

#include "../geometry/rect2d.hpp"
#include "layer_manager.hpp"

namespace yg
{
  namespace gl
  {
    class Clipper : public LayerManager
    {
    private:

      typedef LayerManager base_t;

      bool m_isClippingEnabled;
      m2::RectI m_clipRect;

    public:

      Clipper(base_t::Params const & params);

      void beginFrame();
      void endFrame();

      /// Working with clip rect
      /// @{
      /// enabling/disabling and querying clipping state
      void enableClipRect(bool flag);
      bool clipRectEnabled() const;

      /// Setting and querying clipRect
      void setClipRect(m2::RectI const & rect);
      m2::RectI const & clipRect() const;
      /// @}

    };
  }
}
