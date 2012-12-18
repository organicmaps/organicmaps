#pragma once

#include "opengl/vertex.hpp"
#include "opengl/buffer_object.hpp"
#include "opengl/renderbuffer.hpp"
#include "opengl/framebuffer.hpp"
#include "opengl/storage.hpp"

#include "pipeline_manager.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"

#include "../geometry/angles.hpp"

#include "../base/matrix.hpp"

namespace graphics
{
  class GeometryBatcher : public PipelinesManager
  {
  private:

    typedef PipelinesManager base_t;

    bool m_isAntiAliased;
    bool m_isSynchronized;

    int m_aaShift;

  public:

    /// INTERNAL API! USE WITH CAUTION
    /// @{
    void flush(int pipelineID);
    /// @}

    bool hasRoom(size_t verticesCount, size_t indicesCount, int pipelineID) const;
    int verticesLeft(int pipelineID) const;
    int  indicesLeft(int pipelineID) const;

    GeometryBatcher(base_t::Params const & params);

    void beginFrame();
    void endFrame();

  public:

    /// This functions hide the base_t functions with the same name and signature
    /// to flush(-1) upon calling them
    /// @{
    void enableClipRect(bool flag);
    void setClipRect(m2::RectI const & rect);

    void clear(Color const & c, bool clearRT = true, float depth = 1.0, bool clearDepth = true);
    /// @}

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

    void addTexturedList(m2::PointF const * coords,
                         m2::PointF const * texCoords,
                         m2::PointF const * normalCoords,
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

    void addTexturedListStrided(m2::PointF const * coords,
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
    void drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m);

    void uploadResources(shared_ptr<Resource> const * resources,
                         size_t count,
                         shared_ptr<gl::BaseTexture> const & texture);

    void applyStates();
    void applyBlitStates();
    void applySharpStates();
  };
}
