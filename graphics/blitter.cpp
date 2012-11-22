#include "opengl/base_texture.hpp"
#include "opengl/buffer_object.hpp"
#include "opengl/utils.hpp"
#include "opengl/storage.hpp"
#include "opengl/vertex.hpp"
#include "opengl/texture.hpp"

#include "blitter.hpp"
#include "resource_manager.hpp"
#include "defines.hpp"

#include "../geometry/screenbase.hpp"
#include "../base/logging.hpp"

namespace graphics
{
  Blitter::Blitter(base_t::Params const & params) : base_t(params)
  {
  }

  Blitter::~Blitter()
  {
  }

  void Blitter::blit(shared_ptr<gl::BaseTexture> const & srcSurface,
                     ScreenBase const & from,
                     ScreenBase const & to,
                     bool isSubPixel,
                     graphics::Color const & color,
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

    graphics::gl::Storage storage = resourceManager()->multiBlitStorages()->Reserve();

    if (resourceManager()->multiBlitStorages()->IsCancelled())
    {
      LOG(LINFO, ("skipping multiBlit on cancelled multiBlitStorages pool"));
      return;
    }

    /// TODO : Bad lock/unlock checking pattern. Should refactor
    if (!storage.m_vertices->isLocked())
      storage.m_vertices->lock();
    if (!storage.m_indices->isLocked())
      storage.m_indices->lock();

    gl::Vertex * pointsData = (gl::Vertex*)storage.m_vertices->data();

    for (size_t i = 0; i < s * 4; ++i)
    {
      pointsData[i].pt.x = geomPts[i].x;
      pointsData[i].pt.y = geomPts[i].y;
      pointsData[i].depth = graphics::maxDepth;
      pointsData[i].tex.x = texPts[i].x;
      pointsData[i].tex.y = texPts[i].y;
      pointsData[i].normal.x = 0;
      pointsData[i].normal.y = 0;
      //        pointsData[i].color = graphics::Color(255, 255, 255, 255);
    }

    memcpy(storage.m_indices->data(), &idxData[0], idxData.size() * sizeof(unsigned short));

    base_t::unlockStorage(storage);
    base_t::applyBlitStates();

    for (unsigned i = 0; i < s; ++i)
      base_t::drawGeometry(blitInfo[i].m_srcSurface,
                           storage,
                           4,
                           sizeof(unsigned short) * i * 4,
                           ETrianglesFan);

    base_t::applyStates();
    base_t::discardStorage(storage);

    base_t::freeStorage(storage, resourceManager()->multiBlitStorages());
  }

  void Blitter::blit(shared_ptr<graphics::gl::BaseTexture> const & srcSurface,
                     math::Matrix<double, 3, 3> const & m,
                     bool isSubPixel)
  {
    blit(srcSurface,
         m,
         isSubPixel,
         graphics::Color(),
         m2::RectI(0, 0, srcSurface->width(), srcSurface->height()),
         m2::RectU(0, 0, srcSurface->width(), srcSurface->height()));
  }

  void Blitter::calcPoints(m2::RectI const & srcRect,
                           m2::RectU const & texRect,
                           shared_ptr<gl::BaseTexture> const & srcSurface,
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

  void Blitter::blit(shared_ptr<graphics::gl::BaseTexture> const & srcSurface,
                     math::Matrix<double, 3, 3> const & m,
                     bool isSubPixel,
                     graphics::Color const & color,
                     m2::RectI const & srcRect,
                     m2::RectU const & texRect)
  {
    m2::PointF pts[4];
    m2::PointF texPts[4];

    calcPoints(srcRect, texRect, srcSurface, m, isSubPixel, pts, texPts);

    immDrawTexturedPrimitives(pts, texPts, 4, srcSurface, true, color, false);
  }

  void Blitter::blit(shared_ptr<gl::BaseTexture> const & srcSurface,
                     ScreenBase const & from,
                     ScreenBase const & to,
                     bool isSubPixel)
  {
    blit(srcSurface,
         from,
         to,
         isSubPixel,
         graphics::Color(),
         m2::RectI(0, 0, srcSurface->width(), srcSurface->height()),
         m2::RectU(0, 0, srcSurface->width(), srcSurface->height()));
  }

  void Blitter::immDrawSolidRect(m2::RectF const & rect,
                                 graphics::Color const & color)
  {
    immDrawRect(rect, m2::RectF(), shared_ptr<gl::RGBA8Texture>(), false, color, true);
  }

  void Blitter::immDrawRect(m2::RectF const & rect,
                            m2::RectF const & texRect,
                            shared_ptr<gl::BaseTexture> texture,
                            bool hasTexture,
                            graphics::Color const & color,
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
                                    shared_ptr<gl::BaseTexture> const & texture)
  {
    shared_ptr<IMMDrawTexturedRect> command(new IMMDrawTexturedRect(rect, texRect, texture, resourceManager()));
    processCommand(command);
  }

  void Blitter::immDrawTexturedPrimitives(m2::PointF const * pts,
                                          m2::PointF const * texPts,
                                          size_t size,
                                          shared_ptr<gl::BaseTexture> const & texture,
                                          bool hasTexture,
                                          graphics::Color const & color,
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
}

