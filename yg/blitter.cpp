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
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    Blitter::IMMDrawTexturedRect::IMMDrawTexturedRect(
        m2::RectF const & rect,
        m2::RectF const & texRect,
        shared_ptr<BaseTexture> const & texture,
        shared_ptr<ResourceManager> const & rm)
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

      m_pts.resize(4);
      m_texPts.resize(4);

      copy(rectPoints, rectPoints + 4, &m_pts[0]);
      copy(texRectPoints, texRectPoints + 4, &m_texPts[0]);
      m_ptsCount = 4;
      m_texture = texture;
      m_hasTexture = true;
      m_hasColor = false;
      m_resourceManager = rm;
    }

    Blitter::Blitter(base_t::Params const & params) : base_t(params)
    {
    }

    Blitter::~Blitter()
    {
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
                       bool isSubPixel,
                       yg::Color const & color,
                       m2::RectI const & srcRect,
                       m2::RectU const & texRect)
    {
      blit(srcSurface,
           from.PtoGMatrix() * to.GtoPMatrix(),
           isSubPixel,
           color,
           srcRect,
           texRect);
    }

    void Blitter::blit(BlitInfo const * blitInfo,
                       size_t s,
                       bool isSubPixel)
    {
      vector<m2::PointF> geomPts(4 * s);
      vector<m2::PointF> texPts(4 * s);
      vector<unsigned short> idxData(4 * s);

      for (size_t i = 0; i < s; ++i)
      {
        calcPoints(blitInfo[i].m_srcRect,
                   blitInfo[i].m_texRect,
                   blitInfo[i].m_srcSurface,
                   blitInfo[i].m_matrix,
                   isSubPixel,
                   &geomPts[i * 4],
                   &texPts[i * 4]);

        idxData[i * 4    ] = i * 4;
        idxData[i * 4 + 1] = i * 4 + 1;
        idxData[i * 4 + 2] = i * 4 + 2;
        idxData[i * 4 + 3] = i * 4 + 3;
      }

      yg::gl::Storage storage = resourceManager()->multiBlitStorages()->Reserve();

      AuxVertex * pointsData = (AuxVertex*)storage.m_vertices->lock();

      for (size_t i = 0; i < s * 4; ++i)
      {
        pointsData[i].pt.x = geomPts[i].x;
        pointsData[i].pt.y = geomPts[i].y;
        pointsData[i].texPt.x = texPts[i].x;
        pointsData[i].texPt.y = texPts[i].y;
        pointsData[i].color = yg::Color(255, 255, 255, 255);
      }

      storage.m_vertices->unlock();
      storage.m_vertices->makeCurrent();

      setupAuxVertexLayout(false, true, storage.m_vertices->glPtr());

      OGLCHECK(glDisable(GL_BLEND));
      OGLCHECK(glDisable(GL_DEPTH_TEST));

      memcpy(storage.m_indices->lock(), &idxData[0], idxData.size() * sizeof(unsigned short));

      storage.m_indices->unlock();
      storage.m_indices->makeCurrent();

      for (unsigned i = 0; i < s; ++i)
      {
        if (blitInfo[i].m_srcSurface)
          blitInfo[i].m_srcSurface->makeCurrent();

        OGLCHECK(glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (unsigned short *)storage.m_indices->glPtr() + i * 4));
      }

      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      OGLCHECK(glEnable(GL_BLEND));
//      /// This call is necessary to avoid parasite blitting in updateActualTarget() on IPhone.
//      OGLCHECK(glFinish());

      resourceManager()->multiBlitStorages()->Free(storage);
    }

    void Blitter::blit(shared_ptr<yg::gl::BaseTexture> const & srcSurface,
                       math::Matrix<double, 3, 3> const & m,
                       bool isSubPixel)
    {
      blit(srcSurface,
           m,
           isSubPixel,
           yg::Color(),
           m2::RectI(0, 0, srcSurface->width(), srcSurface->height()),
           m2::RectU(0, 0, srcSurface->width(), srcSurface->height()));
    }

    void Blitter::calcPoints(m2::RectI const & srcRect,
                             m2::RectU const & texRect,
                             shared_ptr<BaseTexture> const & srcSurface,
                             math::Matrix<double, 3, 3> const & m,
                             bool isSubPixel,
                             m2::PointF * geomPts,
                             m2::PointF * texPts)
    {
      m2::PointF pt = m2::PointF(m2::PointD(srcRect.minX(), srcRect.minY()) * m);

      if (!isSubPixel)
      {
        pt.x = pt.x - my::rounds(pt.x);
        pt.y = pt.y - my::rounds(pt.y);
      }
      else
        pt = m2::PointF(0, 0);

      geomPts[0] = m2::PointF(m2::PointD(srcRect.minX(), srcRect.minY()) * m) + pt;
      geomPts[1] = m2::PointF(m2::PointD(srcRect.maxX(), srcRect.minY()) * m) + pt;
      geomPts[2] = m2::PointF(m2::PointD(srcRect.maxX(), srcRect.maxY()) * m) + pt;
      geomPts[3] = m2::PointF(m2::PointD(srcRect.minX(), srcRect.maxY()) * m) + pt;

      texPts[0] = srcSurface->mapPixel(m2::PointF(texRect.minX(), texRect.minY()));
      texPts[1] = srcSurface->mapPixel(m2::PointF(texRect.maxX(), texRect.minY()));
      texPts[2] = srcSurface->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()));
      texPts[3] = srcSurface->mapPixel(m2::PointF(texRect.minX(), texRect.maxY()));
    }

    void Blitter::blit(shared_ptr<yg::gl::BaseTexture> const & srcSurface,
                       math::Matrix<double, 3, 3> const & m,
                       bool isSubPixel,
                       yg::Color const & color,
                       m2::RectI const & srcRect,
                       m2::RectU const & texRect)
    {
      m2::PointF pts[4];
      m2::PointF texPts[4];

      calcPoints(srcRect, texRect, srcSurface, m, isSubPixel, pts, texPts);

      immDrawTexturedPrimitives(pts, texPts, 4, srcSurface, true, color, false);
    }

    void Blitter::blit(shared_ptr<BaseTexture> const & srcSurface,
                       ScreenBase const & from,
                       ScreenBase const & to,
                       bool isSubPixel)
    {
      blit(srcSurface,
           from,
           to,
           isSubPixel,
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
      shared_ptr<IMMDrawTexturedRect> command(new IMMDrawTexturedRect(rect, texRect, texture, resourceManager()));
      processCommand(command);
    }

    void Blitter::immDrawTexturedPrimitives(m2::PointF const * pts,
                                   m2::PointF const * texPts,
                                   size_t size,
                                   shared_ptr<BaseTexture> const & texture,
                                   bool hasTexture,
                                   yg::Color const & color,
                                   bool hasColor)
    {
      shared_ptr<IMMDrawTexturedPrimitives> command(new IMMDrawTexturedPrimitives());

      command->m_ptsCount = size;
      command->m_pts.resize(size);
      command->m_texPts.resize(size);
      copy(pts, pts + size, command->m_pts.begin());
      copy(texPts, texPts + size, command->m_texPts.begin());
      command->m_texture = texture;
      command->m_hasTexture = hasTexture;
      command->m_color = color;
      command->m_hasColor = hasColor;
      command->m_resourceManager = resourceManager();

      processCommand(command);
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

    void Blitter::IMMDrawTexturedPrimitives::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing IMMDrawTexturedPrimitives command"));
      yg::gl::Storage blitStorage = m_resourceManager->blitStorages()->Reserve();

      AuxVertex * pointsData = (AuxVertex*)blitStorage.m_vertices->data();

      for (size_t i = 0; i < m_ptsCount; ++i)
      {
        pointsData[i].pt.x = m_pts[i].x;
        pointsData[i].pt.y = m_pts[i].y;
        pointsData[i].texPt.x = m_texPts[i].x;
        pointsData[i].texPt.y = m_texPts[i].y;
        pointsData[i].color = m_color;
      }

      blitStorage.m_vertices->unlock();
      blitStorage.m_vertices->makeCurrent();

      Blitter::setupAuxVertexLayout(m_hasColor, m_hasTexture, blitStorage.m_vertices->glPtr());

      if (m_texture)
        m_texture->makeCurrent();

      unsigned short idxData[4] = {0, 1, 2, 3};
      memcpy(blitStorage.m_indices->data(), idxData, sizeof(idxData));
      blitStorage.m_indices->unlock();
      blitStorage.m_indices->makeCurrent();

      OGLCHECK(glDisable(GL_BLEND));
      OGLCHECK(glDisable(GL_DEPTH_TEST));
      OGLCHECK(glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, blitStorage.m_indices->glPtr()));
      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      OGLCHECK(glEnable(GL_BLEND));
//      /// This call is necessary to avoid parasite blitting in updateActualTarget() on IPhone.
//      OGLCHECK(glFinish());

      m_resourceManager->blitStorages()->Free(blitStorage);
    }
  }
}

