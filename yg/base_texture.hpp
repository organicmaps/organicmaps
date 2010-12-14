#pragma once

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "render_target.hpp"

namespace yg
{
  class Color;
  namespace gl
  {
    class BaseTexture : public RenderTarget
    {
      private:

        /// OpenGL texture ID
        unsigned m_id;
        /// texture dimensions
        /// @{
        unsigned m_width;
        unsigned m_height;
        /// @}

        void init();

      public:

        BaseTexture(m2::PointU const & size);
        BaseTexture(unsigned width, unsigned height);
        ~BaseTexture();

        unsigned width() const;
        unsigned height() const;

        int id() const;
        void makeCurrent();
        void attachToFrameBuffer();

        m2::PointF const mapPixel(m2::PointU const & p) const;
        m2::RectF const mapRect(m2::RectF const & r) const;
        void mapPixel(float & x, float & y) const;

        virtual void fill(yg::Color const & c) = 0;
        virtual void dump(char const * fileName) = 0;

        static unsigned current();
    };
  }
}
