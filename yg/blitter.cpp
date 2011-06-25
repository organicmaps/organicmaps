#include "../base/SRC_FIRST.hpp"

#include "blitter.hpp"
#include "framebuffer.hpp"
#include "base_texture.hpp"
#include "resource_manager.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "utils.hpp"
#include "storage.hpp"
#include "vertex.hpp"
#include "texture.hpp"

#include "../geometry/screenbase.hpp"

namespace yg
{
  namespace gl
  {
    Blitter::Blitter(base_t::Params const & params) : base_t(params)
    {
//      m_blitStorage = resourceManager()->reserveBlitStorage();
    }

    Blitter::~Blitter()
    {
//      resourceManager()->freeBlitStorage(m_blitStorage);
    }

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
        srcSurface->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
        srcSurface->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
        srcSurface->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY())),
        srcSurface->mapPixel(m2::PointF(texRect.minX(), texRect.maxY()))
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

    void Blitter::setupAuxVertexLayout(bool hasColor, bool hasTexture, void * glPtr)
    {
      OGLCHECK(glEnableClientState(GL_VERTEX_ARRAY));
      OGLCHECK(glVertexPointer(2, GL_FLOAT, sizeof(AuxVertex), (void*)((char*)glPtr + AuxVertex::vertexOffs)));

      if (hasColor)
      {
        OGLCHECK(glEnableClientState(GL_COLOR_ARRAY));
        OGLCHECK(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(AuxVertex), (void*)((char*)glPtr + AuxVertex::colorOffs)));
      }
      else
        OGLCHECK(glDisableClientState(GL_COLOR_ARRAY));

      if (hasTexture)
      {
        OGLCHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        OGLCHECK(glTexCoordPointer(2, GL_FLOAT, sizeof(AuxVertex), (void*)((char*)glPtr + AuxVertex::texCoordsOffs)));
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
      m_blitStorage = resourceManager()->blitStorages().Reserve();

      AuxVertex * pointsData = (AuxVertex*)m_blitStorage.m_vertices->lock();

      for (size_t i = 0; i < size; ++i)
      {
        pointsData[i].pt.x = pts[i].x;
        pointsData[i].pt.y = pts[i].y;
        pointsData[i].texPt.x = texPts[i].x;
        pointsData[i].texPt.y = texPts[i].y;
        pointsData[i].color = color;
      }

      m_blitStorage.m_vertices->unlock();
      m_blitStorage.m_vertices->makeCurrent();

      setupAuxVertexLayout(hasColor, hasTexture, m_blitStorage.m_vertices->glPtr());

      if (texture)
        texture->makeCurrent();

      unsigned short idxData[4] = {0, 1, 2, 3};
      memcpy(m_blitStorage.m_indices->lock(), idxData, sizeof(idxData));
      m_blitStorage.m_indices->unlock();
      m_blitStorage.m_indices->makeCurrent();

      OGLCHECK(glDisable(GL_BLEND));
      OGLCHECK(glDisable(GL_DEPTH_TEST));
      OGLCHECK(glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, m_blitStorage.m_indices->glPtr()));
      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      OGLCHECK(glEnable(GL_BLEND));
      /// This call is necessary to avoid parasite blitting in updateActualTarget() on IPhone.
      OGLCHECK(glFinish());

      resourceManager()->blitStorages().Free(m_blitStorage);
      m_blitStorage = yg::gl::Storage();
    }
  }
}

