#pragma once

#include "graphics/opengl/vertex.hpp"
#include "graphics/opengl/buffer_object.hpp"
#include "graphics/opengl/renderbuffer.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/storage.hpp"

#include "graphics/pipeline_manager.hpp"
#include "graphics/resource_cache.hpp"
#include "graphics/resource_manager.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/list.hpp"
#include "std/function.hpp"

#include "geometry/angles.hpp"

#include "base/matrix.hpp"

namespace graphics
{
  class GeometryBatcher : public PipelinesManager
  {
  private:

    typedef PipelinesManager base_t;

    bool m_isAntiAliased;

    int m_aaShift;

  public:

    /// INTERNAL API! USE WITH CAUTION
    /// @{
    void flush(int pipelineID);
    /// @}

    bool hasRoom(size_t verticesCount, size_t indicesCount, int pipelineID) const;
    int verticesLeft(int pipelineID) const;
    int  indicesLeft(int pipelineID) const;

    struct Params : public base_t::Params
    {
      EStorageType m_storageType;
      ETextureType m_textureType;
      uint32_t m_pipelineCount;

      Params();
    };

    uint8_t m_startStaticPage;
    uint8_t m_staticPagesCount;

    uint8_t m_startDynamicPage;
    uint8_t m_dynamicPage;
    uint8_t m_dynamicPagesCount;

    GeometryBatcher(Params const & params);

    void beginFrame();
    void endFrame();

    bool isDynamicPage(int i) const;
    void flushDynamicPage();
    int  nextDynamicPage() const;
    void changeDynamicPage();

    void onDynamicOverflow(int pipelineID);

    /// copy vertices from source VertexStream
    unsigned copyVertices(VertexStream * srcVS,
                          unsigned vCount,
                          unsigned iCount,
                          int pipelineID);

  public:

    /// This functions hide the base_t functions with the same name and signature
    /// to flush(-1) upon calling them
    /// @{
    void enableClipRect(bool flag);
    void setClipRect(m2::RectI const & rect);

    void clear(Color const & c, bool clearRT = true, float depth = 1.0, bool clearDepth = true);
    /// @}

    unsigned reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                              EStorageType storageType,
                              VertexDecl const * decl);

    unsigned reservePipelines(unsigned count,
                              ETextureType textureType,
                              EStorageType storageType,
                              VertexDecl const * decl);

    void addTriangleStrip(VertexStream * srcVS,
                          unsigned count,
                          int pipelineID);

    void addTriangleList(VertexStream * srcVS,
                         unsigned count,
                         int pipelineID);

    void addTriangleFan(VertexStream * srcVS,
                        unsigned count,
                        int pipelineID);

    void addTexturedFan(m2::PointF const * coords,
                        m2::PointF const * normals,
                        m2::PointF const * texCoords,
                        unsigned size,
                        double depth,
                        int pipelineID);

    void addTexturedFanStrided(m2::PointF const * coords,
                               size_t coordsStride,
                               m2::PointF const * normals,
                               size_t normalsStride,
                               m2::PointF const * texCoords,
                               size_t texCoordsStride,
                               unsigned size,
                               double depth,
                               int pipelineID);

    void addTexturedStrip(m2::PointF const * coords,
                          m2::PointF const * normals,
                          m2::PointF const * texCoords,
                          unsigned size,
                          double depth,
                          int pipelineID);

    void addTexturedStripStrided(m2::PointF const * coords,
                                 size_t coordsStride,
                                 m2::PointF const * normals,
                                 size_t normalsStride,
                                 m2::PointF const * texCoords,
                                 size_t texCoordsStride,
                                 unsigned size,
                                 double depth,
                                 int pipelineID);

    void addTexturedStripStrided(m2::PointD const * coords,
                                 size_t coordsStride,
                                 m2::PointF const * normals,
                                 size_t normalsStride,
                                 m2::PointF const * texCoords,
                                 size_t texCoordsStride,
                                 unsigned size,
                                 double depth,
                                 int pipelineID);

    void addTexturedList(m2::PointF const * coords,
                         m2::PointF const * texCoords,
                         m2::PointF const * normalCoords,
                         unsigned size,
                         double depth,
                         int pipelineID);

    void addTexturedListStrided(m2::PointF const * coords,
                                size_t coordsStride,
                                m2::PointF const * normals,
                                size_t normalsStride,
                                m2::PointF const * texCoords,
                                size_t texCoordsStride,
                                unsigned size,
                                double depth,
                                int pipelineID);

    void addTexturedListStrided(m2::PointD const * coords,
                                size_t coordsStride,
                                m2::PointF const * normals,
                                size_t normalsStride,
                                m2::PointF const * texCoords,
                                size_t texCoordsStride,
                                unsigned size,
                                double depth,
                                int pipelineID);

    int aaShift() const;

    void drawStraightTexturedPolygon(
        m2::PointD const & ptPivot,
        float tx0, float ty0, float tx1, float ty1,
        float x0, float y0, float x1, float y1,
        double depth,
        int pipelineID);

    /// drawing textured polygon with antialiasing
    /// we assume that the (tx0, ty0, tx1, ty1) area on texture is surrounded by (0, 0, 0, 0) pixels,
    /// and the 1px interior area is also (0, 0, 0, 0).
    void drawTexturedPolygon(
        m2::PointD const & ptWhere,
        ang::AngleD const & angle,
        float tx0, float ty0, float tx1, float ty1,
        float x0, float y0, float x1, float y1,
        double depth,
        int pipelineID);

    void setDisplayList(DisplayList * dl);
    void drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m, UniformsHolder * holder = NULL);

    void uploadResources(shared_ptr<Resource> const * resources,
                         size_t count,
                         shared_ptr<gl::BaseTexture> const & texture);

    void applyStates();
    void applyBlitStates();
    void applySharpStates();

    /// map Resource::Info on skin
    /// if found - return id.
    /// if not - pack and return id.
    uint32_t mapInfo(Resource::Info const & info);
    /// map array of Resource::Info's on skin
    bool mapInfo(Resource::Info const * const * infos,
             uint32_t * ids,
             size_t count);

    uint32_t findInfo(Resource::Info const & info);


    uint8_t dynamicPage() const;

    /// change pipeline for its "backbuffer" counterpart.
    /// pipelines works in pairs to employ "double-buffering" technique
    /// to enhance CPU-GPU parallelism.
    /// this function is called after any rendering command
    /// issued to avoid the "GPU is waiting on texture used in
    /// rendering call" issue.
    /// @warning does nothing for pipelines with EStaticTexture type.
    /// (pipelines loaded at screen creation time)
    void changePipeline(int i);
    int nextPipeline(int i) const;
  };
}
