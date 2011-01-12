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

      enum TextPos { under_line, middle_line, above_line };

    private:

      static wstring GetDrawString(string const & utf8Str);

      typedef RenderStateUpdater base_t;

      shared_ptr<yg::Skin> m_skin;

      struct GeometryPipeline
      {
        size_t m_currentVertex;
        size_t m_currentIndex;

        size_t m_maxVertices;
        size_t m_maxIndices;

        Storage m_storage;

        Vertex * m_vertices;
        unsigned short * m_indices;

        size_t verticesLeft();
        size_t indicesLeft();
      };

      vector<GeometryPipeline> m_pipelines;

      bool hasRoom(size_t verticesCount, size_t indicesCount, int pageID) const;

      size_t verticesLeft(int pageID);
      size_t indicesLeft(int pageID);

      void reset(int pageID);
      void flush(int pageID);

      void switchTextures(int pageID);

      /// Apply all states needed for rendering a batch of geometry.
      void applyStates();

      bool m_isAntiAliased;

      int m_aaShift;

      void drawPathTextImpl(m2::PointD const * path,
                        size_t s,
                        uint8_t fontSize,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        TextPos pos,
                        bool fromMask,
                        double depth,
                        bool isFixedFont);

    public:

      struct Params : base_t::Params
      {
        bool m_isAntiAliased;
        Params() : m_isAntiAliased(false) {}
      };

      GeometryBatcher(Params const & params);
      ~GeometryBatcher();

      void setSkin(shared_ptr<Skin> skin);
      shared_ptr<Skin> skin() const;

      void beginFrame();
      void endFrame();

      void drawPoint(m2::PointD const & pt,
                     uint32_t styleID,
                     double depth);
      void drawPath(m2::PointD const * points,
                    size_t pointsCount,
                    uint32_t styleID,
                    double depth);

      void drawTriangles(m2::PointD const * points,
                       size_t pointsCount,
                       uint32_t styleID,
                       double depth);

    private:
      template <class ToDo>
      void ForEachGlyph(uint8_t fontSize, wstring const & text, bool isMask, bool isFixedFont, ToDo toDo);

    public:
      /// Drawing text from point rotated by the angle.
      void drawText(m2::PointD const & pt,
                    float angle,
                    uint8_t fontSize,
                    string const & utf8Text,
                    double depth,
                    bool fixedFont = false);

      m2::RectD const textRect(string const & utf8Text,
                               uint8_t fontSize);

      /// Drawing text in the middle of the path.
      void drawPathText(m2::PointD const * path,
                        size_t s,
                        uint8_t fontSize,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        TextPos pos,
                        bool isMasked,
                        double depth,
                        bool isFixedFont = false);

      /// This functions hide the base_t functions with the same name and signature
      /// to flush(-1) upon calling them
      /// @{
      void enableClipRect(bool flag);
      void setClipRect(m2::RectI const & rect);

      void clear(yg::Color const & c = yg::Color(192, 192, 192, 255), bool clearRT = true, float depth = 1.0, bool clearDepth = true);

      /// @}

      void setRenderTarget(shared_ptr<RenderTarget> const & rt);

      void addTexturedVertices(m2::PointF const * coords,
                               m2::PointF const * texCoords,
                               unsigned size,
                               double depth,
                               int pageID);

      int aaShift() const;

    private:

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     float angle,
                     float blOffset,
                     CharStyle const * p,
                     double depth);

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
    };
  }
}

#include "../base/stop_mem_debug.hpp"
