#pragma once

#include "vertex.hpp"
#include "buffer_object.hpp"
#include "renderbuffer.hpp"
#include "framebuffer.hpp"
#include "blitter.hpp"
#include "storage.hpp"
#include "skin_page.hpp"
#include "resource_manager.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"

#include "../geometry/angles.hpp"

#include "../base/matrix.hpp"

namespace threads
{
  class Mutex;
}

namespace graphics
{
  class Skin;

  namespace gl
  {
    class GeometryBatcher : public Blitter
    {
    public:

      typedef function<void()> onFlushFinishedFn;

    private:

      typedef Blitter base_t;

      shared_ptr<graphics::Skin> m_skin;

      struct GeometryPipeline
      {
        size_t m_verticesDrawn;
        size_t m_indicesDrawn;

        size_t m_currentVertex;
        size_t m_currentIndex;

        /// made mutable to implement lazy reservation of m_storage
        /// @{
        mutable size_t m_maxVertices;
        mutable size_t m_maxIndices;

        mutable bool m_hasStorage;
        mutable Storage m_storage;

        mutable Vertex * m_vertices;
        mutable unsigned short * m_indices;
        /// @}

        bool m_useGuiResources;
        graphics::SkinPage::EType m_type;

        int  verticesLeft();
        int  indicesLeft();

        void checkStorage(shared_ptr<ResourceManager> const & resourceManager) const;
      };

      vector<GeometryPipeline> m_pipelines;

      void reset(int pipelineID);

      void freePipeline(int pipelineID);
      void freeTexture(int pipelineID);

      bool m_isAntiAliased;
      bool m_isSynchronized;
      bool m_useGuiResources;

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
        bool m_useGuiResources;
        Params();
      };

      GeometryBatcher(Params const & params);
      ~GeometryBatcher();

      void setSkin(shared_ptr<Skin> skin);
      shared_ptr<Skin> const & skin() const;

      void beginFrame();
      void endFrame();

      bool flushPipeline(shared_ptr<SkinPage> const & skinPage, int pipelineID);
      void unlockPipeline(int pipelineID);
      void discardPipeline(int pipelineID);

    public:

      /// This functions hide the base_t functions with the same name and signature
      /// to flush(-1) upon calling them
      /// @{
      void enableClipRect(bool flag);
      void setClipRect(m2::RectI const & rect);

      void clear(graphics::Color const & c, bool clearRT = true, float depth = 1.0, bool clearDepth = true);
      /// @}

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);

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

      void memoryWarning();
      void enterBackground();
      void enterForeground();

      void setDisplayList(DisplayList * dl);
      void drawDisplayList(DisplayList * dl, math::Matrix<double, 3, 3> const & m);
      void setPixelPrecision(bool flag);
    };
  }
}
