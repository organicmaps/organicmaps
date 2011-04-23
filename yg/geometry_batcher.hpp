#pragma once

#include "vertex.hpp"
#include "texture.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "renderbuffer.hpp"
#include "framebuffer.hpp"
#include "render_state_updater.hpp"
#include "storage.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/list.hpp"
#include "../std/function.hpp"

#include "../base/matrix.hpp"
#include "../base/start_mem_debug.hpp"

namespace threads
{
  class Mutex;
}

namespace yg
{
  class Skin;
  struct CharStyle;

  namespace gl
  {
    class GeometryBatcher : public RenderStateUpdater
    {
    public:

      typedef function<void()> onFlushFinishedFn;

    private:

      typedef RenderStateUpdater base_t;

      shared_ptr<yg::Skin> m_skin;

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

        size_t verticesLeft();
        size_t indicesLeft();

        void checkStorage(shared_ptr<ResourceManager> const & resourceManager, bool isDynamic) const;
      };

      vector<GeometryPipeline> m_pipelines;

      void reset(int pageID);

      void switchTextures(int pageID);

      /// Apply all states needed for rendering a batch of geometry.
      void applyStates();

      bool m_isAntiAliased;

      int m_aaShift;


    public:

      /// INTERNAL API! USE WITH CAUTION
      void flush(int pageID);
      bool hasRoom(size_t verticesCount, size_t indicesCount, int pageID) const;
      size_t verticesLeft(int pageID);
      size_t indicesLeft(int pageID);

      GeometryBatcher(base_t::Params const & params);
      ~GeometryBatcher();

      void setSkin(shared_ptr<Skin> skin);
      shared_ptr<Skin> skin() const;

      void beginFrame();
      void endFrame();

    public:

      /// This functions hide the base_t functions with the same name and signature
      /// to flush(-1) upon calling them
      /// @{
      void enableClipRect(bool flag);
      void setClipRect(m2::RectI const & rect);

      void clear(yg::Color const & c = yg::Color(187, 187, 187, 255), bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      /// @}

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);

      void addTexturedFan(m2::PointF const * coords,
                          m2::PointF const * texCoords,
                          unsigned size,
                          double depth,
                          int pageID);

      void addTexturedStripStrided(m2::PointF const * coords,
                                   size_t coordsStride,
                                   m2::PointF const * texCoords,
                                   size_t texCoordsStride,
                                   unsigned size,
                                   double depth,
                                   int pageID);

      void addTexturedStrip(m2::PointF const * coords,
                            m2::PointF const * texCoords,
                            unsigned size,
                            double depth,
                            int pageID);

      void addTexturedListStrided(m2::PointD const * coords,
                                  size_t coordsStride,
                                  m2::PointF const * texCoords,
                                  size_t texCoordsStride,
                                  unsigned size,
                                  double depth,
                                  int pageID);

      void addTexturedListStrided(m2::PointF const * coords,
                                  size_t coordsStride,
                                  m2::PointF const * texCoords,
                                  size_t texCoordsStride,
                                  unsigned size,
                                  double depth,
                                  int pageID);

      void addTexturedList(m2::PointF const * coords,
                           m2::PointF const * texCoords,
                           unsigned size,
                           double depth,
                           int pageID);

      int aaShift() const;

      /// drawing textured polygon with antialiasing
      /// we assume that the (tx0, ty0, tx1, ty1) area on texture is surrounded by (0, 0, 0, 0) pixels,
      /// and the 1px interior area is also (0, 0, 0, 0).
      void drawTexturedPolygon(
          m2::PointD const & ptWhere,
          float angle,
          float tx0, float ty0, float tx1, float ty1,
          float x0, float y0, float x1, float y1,
          double depth,
          int pageID);

      void memoryWarning();
      void enterBackground();
      void enterForeground();
    };
  }
}

#include "../base/stop_mem_debug.hpp"
