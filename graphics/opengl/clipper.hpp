#pragma once

#include "geometry/rect2d.hpp"
#include "graphics/opengl/renderer.hpp"

namespace graphics
{
  namespace gl
  {
    class Clipper : public Renderer
    {
    private:

      typedef Renderer base_t;

      bool m_isClippingEnabled;
      m2::RectI m_clipRect;

      void enableClipRectImpl(bool flag);
      void setClipRectImpl(m2::RectI const & rect);

      struct EnableClipRect : public Command
      {
        bool m_flag;
        EnableClipRect(bool flag);
        void perform();
      };

      struct SetClipRect : public Command
      {
        m2::RectI m_rect;
        SetClipRect(m2::RectI const & rect);
        void perform();
      };

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
