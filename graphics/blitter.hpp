#pragma once

#include "opengl/storage.hpp"

#include "geometry_batcher.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/shared_ptr.hpp"

class ScreenBase;

namespace graphics
{
  struct Color;

  namespace gl
  {
    class BaseTexture;
  }

  struct BlitInfo
  {
    shared_ptr<gl::BaseTexture> m_srcSurface;
    math::Matrix<double, 3, 3> m_matrix;
    m2::RectI m_srcRect;
    m2::RectU m_texRect;
  };

  class Blitter : public GeometryBatcher
  {
  protected:

    typedef GeometryBatcher base_t;

    void calcPoints(m2::RectI const & srcRect,
                    m2::RectU const & texRect,
                    shared_ptr<gl::BaseTexture> const & texture,
                    math::Matrix<double, 3, 3> const & m,
                    bool isSubPixel,
                    m2::PointF * geomPts,
                    m2::PointF * texPts);
 public:

    Blitter(base_t::Params const & params);
    ~Blitter();

    /// Immediate mode rendering functions.
    /// they doesn't buffer any data as other functions do, but immediately renders it
    /// @{

    void blit(BlitInfo const * blitInfo,
              size_t s,
              bool isSubPixel);

    void immDrawSolidRect(m2::RectF const & rect,
                          Color const & color);

    void immDrawRect(m2::RectF const & rect,
                     m2::RectF const & texRect,
                     shared_ptr<gl::BaseTexture> texture,
                     bool hasTexture,
                     Color const & color,
                     bool hasColor);

    void immDrawTexturedPrimitives(m2::PointF const * pts,
                                   m2::PointF const * texPts,
                                   size_t size,
                                   shared_ptr<gl::BaseTexture> const & texture,
                                   bool hasTexture,
                                   Color const & color,
                                   bool hasColor);

    void immDrawTexturedRect(m2::RectF const & rect,
                             m2::RectF const & texRect,
                             shared_ptr<gl::BaseTexture> const & texture);

    /// @}
  };
}
