#include "../base/SRC_FIRST.hpp"
#include "../base/ptr_utils.hpp"

#include "blitter.hpp"
#include "framebuffer.hpp"
#include "base_texture.hpp"
#include "texture.hpp"
#include "resource_manager.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "utils.hpp"
#include "storage.hpp"

#include "../geometry/screenbase.hpp"

namespace yg
{
  namespace gl
  {
    Blitter::Blitter(base_t::Params const & params) : base_t(params)
    {}

    void Blitter::beginFrame()
    {
      base_t::beginFrame();
    }

    void Blitter::endFrame()
    {
      base_t::endFrame();
    }

    void Blitter::blit(shared_ptr<BaseTexture> const & srcSurface,
                       ScreenBase const & from,
                       ScreenBase const & to,
                       yg::Color const & color,
                       m2::RectI const & srcRect,
                       m2::RectU const & texRect)
    {
      m2::PointF pt = to.GtoP(from.PtoG(m2::PointF(srcRect.minX(), srcRect.minY())));

      pt.x = pt.x - my::rounds(pt.x);
      pt.y = pt.y - my::rounds(pt.y);

      m2::PointF pts[4] =
      {
        to.GtoP(from.PtoG(m2::PointF(srcRect.minX(), srcRect.minY()))) + pt,
        to.GtoP(from.PtoG(m2::PointF(srcRect.maxX(), srcRect.minY()))) + pt,
        to.GtoP(from.PtoG(m2::PointF(srcRect.maxX(), srcRect.maxY()))) + pt,
        to.GtoP(from.PtoG(m2::PointF(srcRect.minX(), srcRect.maxY()))) + pt
      };

      m2::PointF texPts[4] =
      {
        srcSurface->mapPixel(m2::PointU(texRect.minX(), texRect.minY())),
        srcSurface->mapPixel(m2::PointU(texRect.maxX(), texRect.minY())),
        srcSurface->mapPixel(m2::PointU(texRect.maxX(), texRect.maxY())),
        srcSurface->mapPixel(m2::PointU(texRect.minX(), texRect.maxY()))
      };

      immDrawTexturedPrimitives(pts, texPts, 4, srcSurface, true, color, false);
    }

    void Blitter::blit(shared_ptr<BaseTexture> const & srcSurface,
                       ScreenBase const & from,
                       ScreenBase const & to)
    {
      blit(srcSurface,
           from,
           to,
           yg::Color(),
           m2::RectI(0, 0, srcSurface->width(), srcSurface->height()),
           m2::RectU(0, 0, srcSurface->width(), srcSurface->height()));
    }

    void Blitter::immDrawSolidRect(m2::RectF const & rect,
                                  yg::Color const & color)
    {
      immDrawRect(rect, m2::RectF(), shared_ptr<RGBA8Texture>(), false, color, true);
    }

    void Blitter::immDrawRect(m2::RectF const & rect,
                             m2::RectF const & texRect,
                             shared_ptr<BaseTexture> texture,
                             bool hasTexture,
                             yg::Color const & color,
                             bool hasColor)
    {
      m2::PointF rectPoints[4] =
      {
        m2::PointF(rect.minX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.maxY()),
        m2::PointF(rect.minX(), rect.maxY())
      };

      m2::PointF texRectPoints[4] =
      {
        m2::PointF(texRect.minX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.maxY()),
        m2::PointF(texRect.minX(), texRect.maxY()),
      };

      immDrawTexturedPrimitives(rectPoints, texRectPoints, 4, texture, hasTexture, color, hasColor);
    }

    void Blitter::immDrawTexturedRect(m2::RectF const & rect,
                                     m2::RectF const & texRect,
                                     shared_ptr<BaseTexture> const & texture)
    {
      m2::PointF rectPoints[4] =
      {
        m2::PointF(rect.minX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.maxY()),
        m2::PointF(rect.minX(), rect.maxY())
      };

      m2::PointF texRectPoints[4] =
      {
        m2::PointF(texRect.minX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.maxY()),
        m2::PointF(texRect.minX(), texRect.maxY()),
      };

      immDrawTexturedPrimitives(rectPoints, texRectPoints, 4, texture, true, yg::Color(), false);
    }

    struct AuxVertex
    {
      m2::PointF pt;
      m2::PointF texPt;
      yg::Color color;
      static const int vertexOffs = 0;
      static const int texCoordsOffs = sizeof(m2::PointF);
      static const int colorOffs = sizeof(m2::PointF) + sizeof(m2::PointF);
    };

    void Blitter::setupAuxVertexLayout(bool hasColor, bool hasTexture)
    {
      OGLCHECK(glEnableClientState(GL_VERTEX_ARRAY));
      OGLCHECK(glVertexPointer(2, GL_FLOAT, sizeof(AuxVertex), (void*)AuxVertex::vertexOffs));

      if (hasColor)
      {
        OGLCHECK(glEnableClientState(GL_COLOR_ARRAY));
        OGLCHECK(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(AuxVertex), (void*)AuxVertex::colorOffs));
      }
      else
        OGLCHECK(glDisableClientState(GL_COLOR_ARRAY));

      if (hasTexture)
      {
        OGLCHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        OGLCHECK(glTexCoordPointer(2, GL_FLOAT, sizeof(AuxVertex), (void*)AuxVertex::texCoordsOffs));
      }
      else
      {
        OGLCHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
        OGLCHECK(glDisable(GL_TEXTURE_2D));
      }
    }

    void Blitter::immDrawTexturedPrimitives(m2::PointF const * pts,
                                           m2::PointF const * texPts,
                                           size_t size,
                                           shared_ptr<BaseTexture> const & texture,
                                           bool hasTexture,
                                           yg::Color const & color,
                                           bool hasColor)
    {
      yg::gl::Storage blitStorage = resourceManager()->reserveBlitStorage();

      AuxVertex * pointsData = (AuxVertex*)blitStorage.m_vertices->lock();

      for (size_t i = 0; i < size; ++i)
      {
        pointsData[i].pt.x = pts[i].x;
        pointsData[i].pt.y = pts[i].y;
        pointsData[i].texPt.x = texPts[i].x;
        pointsData[i].texPt.y = texPts[i].y;
        pointsData[i].color = color;
      }

      blitStorage.m_vertices->unlock();

      setupAuxVertexLayout(hasColor, hasTexture);

      if (texture)
        texture->makeCurrent();

      unsigned short idxData[4] = {0, 1, 2, 3};
      memcpy(blitStorage.m_indices->lock(), idxData, sizeof(idxData));
      blitStorage.m_indices->unlock();
      blitStorage.m_indices->makeCurrent();

      resourceManager()->freeBlitStorage(blitStorage);

      OGLCHECK(glDisable(GL_BLEND));
      OGLCHECK(glDisable(GL_DEPTH_TEST));
      OGLCHECK(glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, 0));
      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      OGLCHECK(glEnable(GL_BLEND));
    }
  }
}

