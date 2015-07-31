#include "graphics/geometry_batcher.hpp"
#include "graphics/color.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/resource_cache.hpp"

#include "graphics/opengl/base_texture.hpp"
#include "graphics/opengl/utils.hpp"
#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/gl_render_context.hpp"

#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/mutex.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace graphics
{
  template <typename T>
  void CheckPointLayout()
  {
    static_assert(sizeof(m2::Point<T>) == 2 * sizeof(T), "");
    m2::Point<T> p;
    CHECK_EQUAL(reinterpret_cast<unsigned char*>(&p), reinterpret_cast<unsigned char*>(&p.x), ());
    CHECK_EQUAL(reinterpret_cast<unsigned char*>(&p) + sizeof(T), reinterpret_cast<unsigned char*>(&p.y), ());
  }

  GeometryBatcher::Params::Params()
    : m_storageType(ELargeStorage)
    , m_textureType(ELargeTexture)
  {
    CheckPointLayout<float>();
    CheckPointLayout<double>();
  }

  GeometryBatcher::GeometryBatcher(Params const & p)
    : base_t(p),
      m_isAntiAliased(true)
  {
    base_t::applyStates();

    shared_ptr<ResourceCache> cache;
    ResourceManager::loadSkin(resourceManager(), cache);

    m_staticPagesCount = 1;
    m_startStaticPage = reservePipelines({ cache },
                                         EMediumStorage,
                                         gl::Vertex::getVertexDecl());

    m_dynamicPagesCount = 2;
    m_startDynamicPage = reservePipelines(m_dynamicPagesCount,
                                          p.m_textureType,
                                          p.m_storageType,
                                          gl::Vertex::getVertexDecl());
    m_dynamicPage = m_startDynamicPage;

    /// 1 to turn antialiasing on
    /// 2 to switch it off
    m_aaShift = m_isAntiAliased ? 1 : 2;
  }

  unsigned GeometryBatcher::reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                                             EStorageType storageType,
                                             VertexDecl const * decl)
  {
    unsigned res = base_t::reservePipelines(caches, storageType, decl);

    for (size_t i = res; i < res + caches.size(); ++i)
    {
      addClearPageFn(i, bind(&GeometryBatcher::flush, this, i), 100);
      pipeline(i).addHandlesOverflowFn(bind(&GeometryBatcher::onDynamicOverflow, this, i), 0);
    }

    return res;
  }

  unsigned GeometryBatcher::reservePipelines(unsigned count,
                                             ETextureType textureType,
                                             EStorageType storageType,
                                             VertexDecl const * decl)
  {
    unsigned res = base_t::reservePipelines(count, textureType, storageType, decl);

    for (size_t i = res; i < res + count; ++i)
    {
      addClearPageFn(i, bind(&GeometryBatcher::flush, this, i), 100);
      pipeline(i).addHandlesOverflowFn(bind(&GeometryBatcher::onDynamicOverflow, this, i), 0);
    }

    return res;
  }

  void GeometryBatcher::beginFrame()
  {
    base_t::beginFrame();
    base_t::applyStates();
  }

  void GeometryBatcher::clear(graphics::Color const & c, bool clearRT, float depth, bool clearDepth)
  {
    flush(-1);
    base_t::clear(c, clearRT, depth, clearDepth);
  }

  void GeometryBatcher::endFrame()
  {
    flush(-1);
    base_t::endFrame();
  }

  bool GeometryBatcher::hasRoom(size_t verticesCount, size_t indicesCount, int pipelineID) const
  {
    return pipeline(pipelineID).hasRoom(verticesCount, indicesCount);
  }

  int GeometryBatcher::verticesLeft(int pipelineID) const
  {
    GeometryPipeline const & p = pipeline(pipelineID);
    return p.vxLeft();
  }

  int GeometryBatcher::indicesLeft(int pipelineID) const
  {
    GeometryPipeline const & p = pipeline(pipelineID);
    return p.idxLeft();
  }

  void GeometryBatcher::flush(int pipelineID)
  {
    for (size_t i = pipelinesCount(); i > 0; --i)
    {
      size_t id = i - 1;

      if ((pipelineID == -1) || (id == (size_t)pipelineID))
      {
        if (flushPipeline(id))
        {
          int np = nextPipeline(id);

          if (np != id)
          {
            // reserving texture in advance, before we'll
            // potentially return current texture into the pool.
            // this way we make sure that the reserved texture will
            // be different from the returned one.
            pipeline(np).checkTexture();
          }

          changePipeline(id);
        }
      }
    }
  }

  void GeometryBatcher::drawTexturedPolygon(
      m2::PointD const & ptShift,
      ang::AngleD const & angle,
      float tx0, float ty0, float tx1, float ty1,
      float x0, float y0, float x1, float y1,
      double depth,
      int pipelineID)
  {
    if (!hasRoom(4, 6, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage();
    if (!p.hasStorage())
      return;

    float texMinX = tx0;
    float texMaxX = tx1;
    float texMinY = ty0;
    float texMaxY = ty1;

    shared_ptr<gl::BaseTexture> const & texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    texture->mapPixel(texMinX, texMinY);
    texture->mapPixel(texMaxX, texMaxY);

    /// rotated and translated four points (x0, y0), (x0, y1), (x1, y1), (x1, y0)

    m2::PointF coords[4] =
    {
      m2::PointF(x0 * angle.cos() - y0 * angle.sin() + ptShift.x, x0 * angle.sin() + y0 * angle.cos() + ptShift.y),
      m2::PointF(x0 * angle.cos() - y1 * angle.sin() + ptShift.x, x0 * angle.sin() + y1 * angle.cos() + ptShift.y),
      m2::PointF(x1 * angle.cos() - y1 * angle.sin() + ptShift.x, x1 * angle.sin() + y1 * angle.cos() + ptShift.y),
      m2::PointF(x1 * angle.cos() - y0 * angle.sin() + ptShift.x, x1 * angle.sin() + y0 * angle.cos() + ptShift.y)
    };

    /// Special case. Making straight fonts sharp.
    if (angle.val() == 0)
    {
      float deltaX = coords[0].x - ceil(coords[0].x);
      float deltaY = coords[0].y - ceil(coords[0].y);

      for (size_t i = 0; i < 4; ++i)
      {
        coords[i].x -= deltaX;
        coords[i].y -= deltaY;
      }
    }

    m2::PointF texCoords[4] =
    {
      m2::PointF(texMinX, texMinY),
      m2::PointF(texMinX, texMaxY),
      m2::PointF(texMaxX, texMaxY),
      m2::PointF(texMaxX, texMinY)
    };

    m2::PointF normal(0, 0);

    addTexturedFanStrided(coords, sizeof(m2::PointF),
                          &normal, 0,
                          texCoords, sizeof(m2::PointF),
                          4,
                          depth,
                          pipelineID);
  }

  void GeometryBatcher::drawStraightTexturedPolygon(
      m2::PointD const & ptPivot,
      float tx0, float ty0, float tx1, float ty1,
      float x0, float y0, float x1, float y1,
      double depth,
      int pipelineID)
  {
    if (!hasRoom(4, 6, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage();
    if (!p.hasStorage())
      return;

    float texMinX = tx0;
    float texMaxX = tx1;
    float texMinY = ty0;
    float texMaxY = ty1;

    shared_ptr<gl::BaseTexture> const & texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    texture->mapPixel(texMinX, texMinY);
    texture->mapPixel(texMaxX, texMaxY);

    m2::PointF offsets[4] =
    {
      m2::PointF(x0, y0),
      m2::PointF(x0, y1),
      m2::PointF(x1, y1),
      m2::PointF(x1, y0)
    };

    m2::PointF texCoords[4] =
    {
      m2::PointF(texMinX, texMinY),
      m2::PointF(texMinX, texMaxY),
      m2::PointF(texMaxX, texMaxY),
      m2::PointF(texMaxX, texMinY)
    };

    m2::PointF pv(ptPivot.x, ptPivot.y);

    addTexturedFanStrided(&pv, 0,
                          offsets, sizeof(m2::PointF),
                          texCoords, sizeof(m2::PointF),
                          4,
                          depth,
                          pipelineID);
  }

  unsigned GeometryBatcher::copyVertices(VertexStream * srcVS,
                                     unsigned vCount,
                                     unsigned iCount,
                                     int pipelineID)
  {
    if (!hasRoom(vCount, iCount, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);
    p.checkStorage();

    if (!p.hasStorage())
      return (unsigned)-1;

    ASSERT(iCount > 2, ());

    VertexStream * dstVS = p.vertexStream();

    unsigned res = p.currentVx();

    for (unsigned i = 0; i < vCount; ++i)
    {
      dstVS->copyVertex(srcVS);
      srcVS->advanceVertex(1);
      p.advanceVx(1);
    }

    return res;
  }

  void GeometryBatcher::addTriangleFan(VertexStream * srcVS,
                                       unsigned count,
                                       int pipelineID)
  {
    unsigned vOffset = copyVertices(srcVS,
                                    count,
                                    (count - 2) * 3,
                                    pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    if (vOffset != (unsigned)-1)
    {
      unsigned short * indices = (unsigned short*)p.idxData();

      unsigned iOffset = p.currentIdx();

      for (size_t j = 0; j < count - 2; ++j)
      {
        indices[iOffset + j * 3] = vOffset;
        indices[iOffset + j * 3 + 1] = vOffset + j + 1;
        indices[iOffset + j * 3 + 2] = vOffset + j + 2;
      }

      p.advanceIdx((count - 2) * 3);
    }
  }

  void GeometryBatcher::addTriangleStrip(VertexStream * srcVS,
                                         unsigned count,
                                         int pipelineID)
  {
    unsigned vOffset = copyVertices(srcVS,
                                    count,
                                    (count - 2) * 3,
                                    pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    if (vOffset != (unsigned)-1)
    {
      unsigned short * indices = (unsigned short*)p.idxData();

      unsigned iOffset = p.currentIdx();

      size_t oldIdx1 = vOffset;
      size_t oldIdx2 = vOffset + 1;

      for (size_t j = 0; j < count - 2; ++j)
      {
        indices[iOffset + j * 3] = oldIdx1;
        indices[iOffset + j * 3 + 1] = oldIdx2;
        indices[iOffset + j * 3 + 2] = vOffset + j + 2;

        oldIdx1 = oldIdx2;
        oldIdx2 = vOffset + j + 2;
      }

      p.advanceIdx((count - 2) * 3);
    }
  }

  void GeometryBatcher::addTriangleList(VertexStream * srcVS,
                                        unsigned count,
                                        int pipelineID)
  {
    unsigned vOffset = copyVertices(srcVS,
                                    count,
                                    count,
                                    pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    if (vOffset != (unsigned)-1)
    {
      unsigned short * indices = (unsigned short*)p.idxData();

      unsigned iOffset = p.currentIdx();

      for (size_t j = 0; j < count; ++j)
        indices[iOffset + j] = vOffset + j;

      p.advanceIdx(count);
    }

  }

  void GeometryBatcher::addTexturedFan(m2::PointF const * coords,
                                       m2::PointF const * normals,
                                       m2::PointF const * texCoords,
                                       unsigned size,
                                       double depth,
                                       int pipelineID)
  {
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_fPos.m_x = (float*)(coords);
    vs.m_fPos.m_xStride = sizeof(m2::PointF);
    vs.m_fPos.m_y = (float*)(coords) + 1;
    vs.m_fPos.m_yStride = sizeof(m2::PointF);
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = sizeof(m2::PointF);
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = sizeof(m2::PointF);
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = sizeof(m2::PointF);
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = sizeof(m2::PointF);

    addTriangleFan(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedFanStrided(m2::PointF const * coords,
                                              size_t coordsStride,
                                              m2::PointF const * normals,
                                              size_t normalsStride,
                                              m2::PointF const * texCoords,
                                              size_t texCoordsStride,
                                              unsigned size,
                                              double depth,
                                              int pipelineID)
  {
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_fPos.m_x = (float*)(coords);
    vs.m_fPos.m_xStride = coordsStride;
    vs.m_fPos.m_y = (float*)(coords) + 1;
    vs.m_fPos.m_yStride = coordsStride;
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = normalsStride;
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = normalsStride;
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = texCoordsStride;
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = texCoordsStride;

    addTriangleFan(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedStrip(
      m2::PointF const * coords,
      m2::PointF const * normals,
      m2::PointF const * texCoords,
      unsigned size,
      double depth,
      int pipelineID
      )
  {
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_fPos.m_x = (float*)(coords);
    vs.m_fPos.m_xStride = sizeof(m2::PointF);
    vs.m_fPos.m_y = (float*)(coords) + 1;
    vs.m_fPos.m_yStride = sizeof(m2::PointF);
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = sizeof(m2::PointF);
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = sizeof(m2::PointF);
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = sizeof(m2::PointF);
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = sizeof(m2::PointF);

    addTriangleStrip(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedStripStrided(
      m2::PointF const * coords,
      size_t coordsStride,
      m2::PointF const * normals,
      size_t normalsStride,
      m2::PointF const * texCoords,
      size_t texCoordsStride,
      unsigned size,
      double depth,
      int pipelineID)
  {
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_fPos.m_x = (float*)(coords);
    vs.m_fPos.m_xStride = coordsStride;
    vs.m_fPos.m_y = (float*)(coords) + 1;
    vs.m_fPos.m_yStride = coordsStride;
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = normalsStride;
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = normalsStride;
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = texCoordsStride;
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = texCoordsStride;

    addTriangleStrip(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedStripStrided(
      m2::PointD const * coords,
      size_t coordsStride,
      m2::PointF const * normals,
      size_t normalsStride,
      m2::PointF const * texCoords,
      size_t texCoordsStride,
      unsigned size,
      double depth,
      int pipelineID)
  {
    static_assert(sizeof(m2::PointD) == 2 * sizeof(double), "");
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_dPos.m_x = (double*)(coords);
    vs.m_dPos.m_xStride = coordsStride;
    vs.m_dPos.m_y = (double*)(coords) + 1;
    vs.m_dPos.m_yStride = coordsStride;
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = normalsStride;
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = normalsStride;
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = texCoordsStride;
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = texCoordsStride;

    addTriangleStrip(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedListStrided(
      m2::PointD const * coords,
      size_t coordsStride,
      m2::PointF const * normals,
      size_t normalsStride,
      m2::PointF const * texCoords,
      size_t texCoordsStride,
      unsigned size,
      double depth,
      int pipelineID)
  {
    static_assert(sizeof(m2::PointD) == 2 * sizeof(double), "");
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_dPos.m_x = (double*)(coords);
    vs.m_dPos.m_xStride = coordsStride;
    vs.m_dPos.m_y = (double*)(coords) + 1;
    vs.m_dPos.m_yStride = coordsStride;
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = normalsStride;
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = normalsStride;
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = texCoordsStride;
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = texCoordsStride;

    addTriangleList(&vs, size, pipelineID);
  }


  void GeometryBatcher::addTexturedListStrided(
      m2::PointF const * coords,
      size_t coordsStride,
      m2::PointF const * normals,
      size_t normalsStride,
      m2::PointF const * texCoords,
      size_t texCoordsStride,
      unsigned size,
      double depth,
      int pipelineID)
  {
    static_assert(sizeof(m2::PointF) == 2 * sizeof(float), "");

    VertexStream vs;

    vs.m_fPos.m_x = (float*)(coords);
    vs.m_fPos.m_xStride = coordsStride;
    vs.m_fPos.m_y = (float*)(coords) + 1;
    vs.m_fPos.m_yStride = coordsStride;
    vs.m_dPos.m_z = (double*)(&depth);
    vs.m_dPos.m_zStride = 0;
    vs.m_fNormal.m_x = (float*)(normals);
    vs.m_fNormal.m_xStride = normalsStride;
    vs.m_fNormal.m_y = (float*)(normals) + 1;
    vs.m_fNormal.m_yStride = normalsStride;
    vs.m_fTex.m_u = (float*)(texCoords);
    vs.m_fTex.m_uStride = texCoordsStride;
    vs.m_fTex.m_v = (float*)(texCoords) + 1;
    vs.m_fTex.m_vStride = texCoordsStride;

    addTriangleList(&vs, size, pipelineID);
  }

  void GeometryBatcher::addTexturedList(m2::PointF const * coords,
                                        m2::PointF const * normals,
                                        m2::PointF const * texCoords,
                                        unsigned size,
                                        double depth,
                                        int pipelineID)
  {
    addTexturedListStrided(coords, sizeof(m2::PointF),
                           normals, sizeof(m2::PointF),
                           texCoords, sizeof(m2::PointF),
                           size, depth, pipelineID);
  }

  void GeometryBatcher::enableClipRect(bool flag)
  {
    flush(-1);
    base_t::enableClipRect(flag);
  }

  void GeometryBatcher::setClipRect(m2::RectI const & rect)
  {
    flush(-1);
    base_t::setClipRect(rect);
  }

  int GeometryBatcher::aaShift() const
  {
    return m_aaShift;
  }

  void GeometryBatcher::setDisplayList(DisplayList * dl)
  {
    flush(-1);
    base_t::setDisplayList(dl);
  }

  void GeometryBatcher::drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m,
                                        UniformsHolder * holder, size_t indicesCount)
  {
    flush(-1);
    base_t::drawDisplayList(dl, m, holder, indicesCount);
  }

  void GeometryBatcher::uploadResources(shared_ptr<Resource> const * resources,
                                        size_t count,
                                        shared_ptr<gl::BaseTexture> const & texture)
  {
    /// splitting the whole queue of commands into the chunks no more
    /// than 64KB of uploadable data each

    size_t bytesUploaded = 0;
    size_t bytesPerPixel = graphics::formatSize(resourceManager()->params().m_texFormat);
    size_t prev = 0;

    for (size_t i = 0; i < count; ++i)
    {
      shared_ptr<Resource> const & res = resources[i];

      bytesUploaded += res->m_texRect.SizeX() * res->m_texRect.SizeY() * bytesPerPixel;

      if (bytesUploaded > 64 * 1024)
      {
        base_t::uploadResources(resources + prev, i + 1 - prev, texture);
        if (i + 1 < count)
          addCheckPoint();

        prev = i + 1;
        bytesUploaded = 0;
      }
    }

    if (count != 0)
    {
      base_t::uploadResources(resources, count, texture);
      bytesUploaded = 0;
    }
  }

  void GeometryBatcher::applyStates()
  {
    flush(-1);
    base_t::applyStates();
  }

  void GeometryBatcher::applyBlitStates()
  {
    flush(-1);
    base_t::applyBlitStates();
  }

  void GeometryBatcher::applySharpStates()
  {
    flush(-1);
    base_t::applySharpStates();
  }

  bool GeometryBatcher::isDynamicPage(int i) const
  {
    return (i >= m_startDynamicPage) && (i < m_startDynamicPage + m_dynamicPagesCount);
  }

  void GeometryBatcher::flushDynamicPage()
  {
    int currentDynamicPage = m_dynamicPage;
    callClearPageFns(m_dynamicPage);
    if (currentDynamicPage != m_dynamicPage)
      changeDynamicPage();
  }

  int GeometryBatcher::nextDynamicPage() const
  {
    if (m_dynamicPage == m_startDynamicPage + m_dynamicPagesCount - 1)
      return m_startDynamicPage;
    else
      return m_dynamicPage + 1;
  }

  void GeometryBatcher::changeDynamicPage()
  {
    m_dynamicPage = nextDynamicPage();
  }

  int GeometryBatcher::nextPipeline(int i) const
  {
    ASSERT(i < pipelinesCount(), ());

    if (isDynamicPage(i))
      return nextDynamicPage();

    /// for static and text pages return same index as passed in.
    return i;
  }

  void GeometryBatcher::changePipeline(int i)
  {
    if (isDynamicPage(i))
      changeDynamicPage();
  }

  /// This function is set to perform as a callback on texture or handles overflow
  /// BUT! Never called on texture overflow, as this situation
  /// is explicitly checked in the mapXXX() functions.
  void GeometryBatcher::onDynamicOverflow(int pipelineID)
  {
    LOG(LDEBUG, ("DynamicPage flushing, pipelineID=", (uint32_t)pipelineID));
    flushDynamicPage();
  }

  uint8_t GeometryBatcher::dynamicPage() const
  {
    return m_dynamicPage;
  }

  uint32_t GeometryBatcher::mapInfo(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < pipelinesCount(); ++i)
    {
      ResourceCache * cache = pipeline(i).cache().get();
      res = cache->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
      else
      {
        /// trying to find cacheKey
        res = cache->findInfo(info.cacheKey());
        if (res != invalidPageHandle())
        {
          res = cache->addParentInfo(info);
          return packID(i, res);
        }
      }
    }

    if (!pipeline(m_dynamicPage).cache()->hasRoom(info))
      flushDynamicPage();

    return packID(m_dynamicPage,
                  pipeline(m_dynamicPage).cache()->mapInfo(info));
  }

  uint32_t GeometryBatcher::findInfo(Resource::Info const & info)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < pipelinesCount(); ++i)
    {
      res = pipeline(i).cache()->findInfo(info);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    return res;
  }

  bool GeometryBatcher::mapInfo(Resource::Info const * const * infos,
                                 uint32_t * ids,
                                 size_t count)
  {
    for (unsigned cycles = 0; cycles < 2; ++cycles)
    {
      bool packed = true;
      for (unsigned i = 0; i < count; ++i)
      {
        ResourceCache * staticCache = pipeline(m_startStaticPage).cache().get();
        ResourceCache * dynamicCache = pipeline(m_dynamicPage).cache().get();

        ids[i] = staticCache->findInfo(*infos[i]);

        if (ids[i] != invalidPageHandle())
        {
          ids[i] = packID(m_startStaticPage, ids[i]);
          continue;
        }

        ids[i] = staticCache->findInfo(infos[i]->cacheKey());
        if (ids[i] != invalidPageHandle())
        {
          ids[i] = staticCache->addParentInfo(*infos[i]);
          ids[i] = packID(m_startStaticPage, ids[i]);
          continue;
        }

        ids[i] = dynamicCache->findInfo(*infos[i]);
        if (ids[i] != invalidPageHandle())
        {
          ids[i] = packID(m_dynamicPage, ids[i]);
        }
        else
        {
          if (dynamicCache->hasRoom(*infos[i]))
            ids[i] = packID(m_dynamicPage, dynamicCache->mapInfo(*infos[i]));
          else
          {
            packed = false;
            break;
          }
        }
      }

      if (packed)
        return true;
      else
      {
#ifdef DEBUG
        int lastPage = m_dynamicPage;
#endif
        flushDynamicPage();
        ASSERT(lastPage == m_dynamicPage, ());
      }
    }

    return false;
  }

} // namespace graphics
