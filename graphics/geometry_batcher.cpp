#include "geometry_batcher.hpp"
#include "color.hpp"
#include "resource_manager.hpp"
#include "resource_cache.hpp"

#include "opengl/base_texture.hpp"
#include "opengl/utils.hpp"
#include "opengl/opengl.hpp"
#include "opengl/gl_render_context.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/assert.hpp"
#include "../base/math.hpp"
#include "../base/mutex.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

namespace graphics
{
  GeometryBatcher::Params::Params()
    : m_useGuiResources(false)
  {}

  GeometryBatcher::GeometryBatcher(Params const & params)
    : base_t(params),
      m_isAntiAliased(true),
      m_useGuiResources(params.m_useGuiResources)
  {
    base_t::applyStates();

    /// TODO: Perform this after full initialization.
    for (size_t i = 0; i < pipelinesCount(); ++i)
    {
      if (m_useGuiResources)
      {
        GeometryPipeline & p = pipeline(i);
        if (p.textureType() != EStaticTexture)
        {
          p.setTextureType(ESmallTexture);
          p.setStorageType(ESmallStorage);
        }
      }

      addClearPageFn(i, bind(&GeometryBatcher::flush, this, i), 100);
    }

    /// 1 to turn antialiasing on
    /// 2 to switch it off
    m_aaShift = m_isAntiAliased ? 1 : 2;
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
    GeometryPipeline const & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return false;

    return ((p.m_currentVertex + verticesCount <= p.m_maxVertices)
            && (p.m_currentIndex + indicesCount <= p.m_maxIndices));
  }

  int GeometryBatcher::verticesLeft(int pipelineID) const
  {
    GeometryPipeline const & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return -1;

    return p.m_maxVertices - p.m_currentVertex;
  }

  int GeometryBatcher::indicesLeft(int pipelineID) const
  {
    GeometryPipeline const & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return -1;

    return p.m_maxIndices - p.m_currentIndex;
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
          int np = nextPage(id);

          if (np != id)
          {
            // reserving texture in advance, before we'll
            // potentially return current texture into the pool.
            pipeline(np).m_cache->checkTexture();
          }

          changePage(id);
        }

        /// resetting geometry storage associated
        /// with the specified pipeline.
        reset(id);
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

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    float texMinX = tx0;
    float texMaxX = tx1;
    float texMinY = ty0;
    float texMaxY = ty1;

    shared_ptr<gl::BaseTexture> const & texture = p.m_cache->texture();

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

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    float texMinX = tx0;
    float texMaxX = tx1;
    float texMinY = ty0;
    float texMaxY = ty1;

    shared_ptr<gl::BaseTexture> const & texture = p.m_cache->texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    texture->mapPixel(texMinX, texMinY);
    texture->mapPixel(texMaxX, texMaxY);

    /// rotated and translated four points (x0, y0), (x0, y1), (x1, y1), (x1, y0)

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


  void GeometryBatcher::addTexturedFan(m2::PointF const * coords,
                                       m2::PointF const * normals,
                                       m2::PointF const * texCoords,
                                       unsigned size,
                                       double depth,
                                       int pipelineID)
  {
    addTexturedFanStrided(coords, sizeof(m2::PointF),
                          normals, sizeof(m2::PointF),
                          texCoords, sizeof(m2::PointF),
                          size,
                          depth,
                          pipelineID);
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
    if (!hasRoom(size, (size - 2) * 3, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    ASSERT(size > 2, ());

    size_t vOffset = p.m_currentVertex;
    size_t iOffset = p.m_currentIndex;

    for (unsigned i = 0; i < size; ++i)
    {
      p.m_vertices[vOffset + i].pt = *coords;
      p.m_vertices[vOffset + i].normal = *normals;
      p.m_vertices[vOffset + i].tex = *texCoords;
      p.m_vertices[vOffset + i].depth = depth;
      coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
      normals = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(normals) + normalsStride);
      texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
    }

    p.m_currentVertex += size;

    for (size_t j = 0; j < size - 2; ++j)
    {
      p.m_indices[iOffset + j * 3] = vOffset;
      p.m_indices[iOffset + j * 3 + 1] = vOffset + j + 1;
      p.m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;
    }

    p.m_currentIndex += (size - 2) * 3;
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
    addTexturedStripStrided(coords, sizeof(m2::PointF),
                            normals, sizeof(m2::PointF),
                            texCoords, sizeof(m2::PointF),
                            size,
                            depth,
                            pipelineID);
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
    if (!hasRoom(size, (size - 2) * 3, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    ASSERT(size > 2, ());

    size_t vOffset = p.m_currentVertex;
    size_t iOffset = p.m_currentIndex;

    for (unsigned i = 0; i < size; ++i)
    {
      p.m_vertices[vOffset + i].pt = *coords;
      p.m_vertices[vOffset + i].normal = *normals;
      p.m_vertices[vOffset + i].tex = *texCoords;
      p.m_vertices[vOffset + i].depth = depth;
      coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
      normals = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(normals) + normalsStride);
      texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
    }

    p.m_currentVertex += size;

    size_t oldIdx1 = vOffset;
    size_t oldIdx2 = vOffset + 1;

    for (size_t j = 0; j < size - 2; ++j)
    {
      p.m_indices[iOffset + j * 3] = oldIdx1;
      p.m_indices[iOffset + j * 3 + 1] = oldIdx2;
      p.m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;

      oldIdx1 = oldIdx2;
      oldIdx2 = vOffset + j + 2;
    }

    p.m_currentIndex += (size - 2) * 3;
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
    if (!hasRoom(size, size, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    ASSERT(size > 2, ());

    size_t vOffset = p.m_currentVertex;
    size_t iOffset = p.m_currentIndex;

    for (size_t i = 0; i < size; ++i)
    {
      p.m_vertices[vOffset + i].pt = m2::PointF(coords->x, coords->y);
      p.m_vertices[vOffset + i].normal = *normals;
      p.m_vertices[vOffset + i].tex = *texCoords;
      p.m_vertices[vOffset + i].depth = depth;
      coords = reinterpret_cast<m2::PointD const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
      normals = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(normals) + normalsStride);
      texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
    }

    p.m_currentVertex += size;

    for (size_t i = 0; i < size; ++i)
      p.m_indices[iOffset + i] = vOffset + i;

    p.m_currentIndex += size;
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
    if (!hasRoom(size, size, pipelineID))
      flush(pipelineID);

    GeometryPipeline & p = pipeline(pipelineID);

    p.checkStorage(resourceManager());
    if (!p.m_hasStorage)
      return;

    ASSERT(size > 2, ());

    size_t vOffset = p.m_currentVertex;
    size_t iOffset = p.m_currentIndex;

    for (size_t i = 0; i < size; ++i)
    {
      p.m_vertices[vOffset + i].pt = *coords;
      p.m_vertices[vOffset + i].normal = *normals;
      p.m_vertices[vOffset + i].tex = *texCoords;
      p.m_vertices[vOffset + i].depth = depth;
      coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
      normals = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(normals) + normalsStride);
      texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
    }

    p.m_currentVertex += size;

    for (size_t i = 0; i < size; ++i)
      p.m_indices[iOffset + i] = vOffset + i;

    p.m_currentIndex += size;
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

  void GeometryBatcher::drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m)
  {
    flush(-1);
    base_t::drawDisplayList(dl, m);
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
} // namespace graphics
