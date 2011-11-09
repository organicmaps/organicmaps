#pragma once

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "render_target.hpp"

namespace yg
{
  struct Color;
  namespace gl
  {
    class BaseTexture : public RenderTarget
    {
      private:

        /// OpenGL texture ID
        mutable unsigned m_id;
        mutable bool m_hasID;
        /// texture dimensions
        /// @{
        unsigned m_width;
        unsigned m_height;
        /// @}

        void init() const;
        void checkID() const;

      public:

        BaseTexture(m2::PointU const & size);
        BaseTexture(unsigned width, unsigned height);
        ~BaseTexture();

        unsigned width() const;
        unsigned height() const;

        unsigned id() const;
        void makeCurrent() const;
        void attachToFrameBuffer();

        m2::PointF const mapPixel(m2::PointF const & p) const;
        m2::RectF const mapRect(m2::RectF const & r) const;
        void mapPixel(float & x, float & y) const;

        virtual void fill(yg::Color const & c) = 0;
        virtual void dump(char const * fileName) = 0;

        static unsigned current();
    };
  }
}
