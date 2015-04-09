#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include "graphics/render_target.hpp"
#include "graphics/packets_queue.hpp"

namespace graphics
{
  struct Color;
  class PacketsQueue;

  namespace gl
  {
    class BaseTexture : public RenderTarget
    {
      private:

        /// OpenGL texture ID
        mutable unsigned m_id;
        /// texture dimensions
        /// @{
        unsigned m_width;
        unsigned m_height;
        /// @}

        void init() const;

      public:

        BaseTexture(m2::PointU const & size);
        BaseTexture(unsigned width, unsigned height);
        ~BaseTexture();

        unsigned width() const;
        unsigned height() const;

        unsigned id() const;
        void makeCurrent(graphics::PacketsQueue * queue = 0) const;
        void attachToFrameBuffer();
        void detachFromFrameBuffer();

        m2::PointF const mapPixel(m2::PointF const & p) const;
        m2::RectF const mapRect(m2::RectF const & r) const;
        void mapPixel(float & x, float & y) const;

        virtual void fill(graphics::Color const & c) = 0;
        virtual void dump(char const * fileName) = 0;

        static unsigned current();
    };
  }
}
