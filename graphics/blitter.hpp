#pragma once

#include "geometry_renderer.hpp"
#include "storage.hpp"

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

    struct BlitInfo
    {
      shared_ptr<BaseTexture> m_srcSurface;
      math::Matrix<double, 3, 3> m_matrix;
      m2::RectI m_srcRect;
      m2::RectU m_texRect;
    };

    class Blitter : public GeometryRenderer
    {
    private:

      graphics::gl::Storage m_blitStorage;

    protected:

      typedef GeometryRenderer base_t;

      void calcPoints(m2::RectI const & srcRect,
                      m2::RectU const & texRect,
                      shared_ptr<BaseTexture> const & texture,
                      math::Matrix<double, 3, 3> const & m,
                      bool isSubPixel,
                      m2::PointF * geomPts,
                      m2::PointF * texPts);

    public:

      struct IMMDrawTexturedPrimitives : Command
      {
        buffer_vector<m2::PointF, 8> m_pts;
        buffer_vector<m2::PointF, 8> m_texPts;
        unsigned m_ptsCount;
        shared_ptr<BaseTexture> m_texture;
        bool m_hasTexture;
        graphics::Color m_color;
        bool m_hasColor;

        shared_ptr<ResourceManager> m_resourceManager;

        void perform();
      };

      struct IMMDrawTexturedRect : IMMDrawTexturedPrimitives
      {
        IMMDrawTexturedRect(m2::RectF const & rect,
                            m2::RectF const & texRect,
                            shared_ptr<BaseTexture> const & texture,
                            shared_ptr<ResourceManager> const & rm);
      };

      Blitter(base_t::Params const & params);
      ~Blitter();

      void beginFrame();
      void endFrame();

      /// Immediate mode rendering functions.
      /// they doesn't buffer any data as other functions do, but immediately renders it
      /// @{

      void blit(shared_ptr<BaseTexture> const & srcSurface,
                ScreenBase const & from,
                ScreenBase const & to,
                bool isSubPixel,
                graphics::Color const & color,
                m2::RectI const & srcRect,
                m2::RectU const & texRect);

      void blit(shared_ptr<BaseTexture> const & srcSurface,
                ScreenBase const & from,
                ScreenBase const & to,
                bool isSubPixel = false);

      void blit(shared_ptr<BaseTexture> const & srcSurface,
                math::Matrix<double, 3, 3> const & m,
                bool isSubPixel = false);

      void blit(shared_ptr<BaseTexture> const & srcSurface,
                math::Matrix<double, 3, 3> const & m,
                bool isSubPixel,
                graphics::Color const & color,
                m2::RectI const & srcRect,
                m2::RectU const & texRect);

      void blit(BlitInfo const * blitInfo,
                size_t s,
                bool isSubPixel);

      void immDrawSolidRect(m2::RectF const & rect,
                            graphics::Color const & color);

      void immDrawRect(m2::RectF const & rect,
                       m2::RectF const & texRect,
                       shared_ptr<BaseTexture> texture,
                       bool hasTexture,
                       graphics::Color const & color,
                       bool hasColor);

      void immDrawTexturedPrimitives(m2::PointF const * pts,
                                     m2::PointF const * texPts,
                                     size_t size,
                                     shared_ptr<BaseTexture> const & texture,
                                     bool hasTexture,
                                     graphics::Color const & color,
                                     bool hasColor);

      void immDrawTexturedRect(m2::RectF const & rect,
                               m2::RectF const & texRect,
                               shared_ptr<BaseTexture> const & texture);

      /// @}
    };
  }
}
