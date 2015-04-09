#include "graphics/opengl/base_texture.hpp"
#include "graphics/opengl/buffer_object.hpp"
#include "graphics/opengl/utils.hpp"
#include "graphics/opengl/storage.hpp"
#include "graphics/opengl/vertex.hpp"
#include "graphics/opengl/texture.hpp"

#include "graphics/blitter.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/defines.hpp"

#include "geometry/screenbase.hpp"

#include "base/logging.hpp"

#include "std/cstring.hpp"


namespace graphics
{
  Blitter::Blitter(base_t::Params const & params) : base_t(params)
  {
  }

  Blitter::~Blitter()
  {
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

    TStoragePool * storagePool = resourceManager()->storagePool(ESmallStorage);
    gl::Storage storage = storagePool->Reserve();

    if (storagePool->IsCancelled())
    {
      LOG(LDEBUG, ("skipping multiBlit on cancelled multiBlitStorages pool"));
      return;
    }

    /// TODO : Bad lock/unlock checking pattern. Should refactor
    if (!storage.m_vertices->isLocked())
      storage.m_vertices->lock();
    if (!storage.m_indices->isLocked())
      storage.m_indices->lock();

    gl::Vertex * pointsData = (gl::Vertex*)storage.m_vertices->data();

    for (size_t i = 0; i < s; ++i)
    {
      for (size_t j = i * 4; j < (i + 1) * 4; ++j)
      {
        pointsData[j].pt.x = geomPts[j].x;
        pointsData[j].pt.y = geomPts[j].y;
        pointsData[j].depth = blitInfo[i].m_depth;
        pointsData[j].tex.x = texPts[j].x;
        pointsData[j].tex.y = texPts[j].y;
        pointsData[j].normal.x = 0;
        pointsData[j].normal.y = 0;
      }
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

    base_t::freeStorage(storage, storagePool);
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
}

