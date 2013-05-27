#pragma once

#include "area_renderer.hpp"
#include "../geometry/point2d.hpp"

namespace graphics
{
  struct Pen;

  class PathRenderer : public AreaRenderer
  {
  private:

    unsigned m_pathCount;
    unsigned m_pointsCount;
    bool m_drawPathes;
    bool m_useNormals;

    void drawSolidPath(m2::PointD const * pts, size_t ptsCount, double offset, Pen const * pen, double depth);
    void drawStipplePath(m2::PointD const * pts, size_t ptsCount, double offset, Pen const * pen, double depth);
    void drawSymbolPath(m2::PointD const * pts, size_t ptsCount, double offset, Pen const * pen, double depth);

  public:

    typedef AreaRenderer base_t;

    struct Params : base_t::Params
    {
      bool m_drawPathes;
      bool m_useNormals;
      Params();
    };

    PathRenderer(Params const & params);

    void drawPath(m2::PointD const * pts, size_t ptsCount, double offset, uint32_t resID, double depth);

    void beginFrame();
    void endFrame();

    void setUseNormals(bool flag);
  };
}
