#include "../../base/SRC_FIRST.hpp"

#include "../cell_id.hpp"

#include "../../3party/sgitess/interface.h"

namespace feature
{
  struct AddTessPointF
  {
    tess::Tesselator & m_tess;
    AddTessPointF(tess::Tesselator & tess) : m_tess(tess)
    {}
    void operator()(m2::PointD const & p)
    {
      m_tess.add(tess::Vertex(p.x, p.y));
    }
  };

  typedef vector<m2::PointD> points_t;

  void TesselateInterior( points_t const & bound, list<points_t> const & holes,
                          points_t & triangles)
  {
    tess::VectorDispatcher disp;
    tess::Tesselator tess;
    tess.setDispatcher(&disp);
    tess.setWindingRule(tess::WindingOdd);

    tess.beginPolygon();

    tess.beginContour();
    for_each(bound.begin(), bound.end(), AddTessPointF(tess));
    tess.endContour();

    for (list<points_t>::const_iterator it = holes.begin(); it != holes.end(); ++it)
    {
      tess.beginContour();
      for_each(it->begin(), it->end(), AddTessPointF(tess));
      tess.endContour();
    }

    tess.endPolygon();

    for (size_t i = 0; i < disp.indices().size(); ++i)
    {
      switch (disp.indices()[i].first)
      {
      case tess::TrianglesFan:
      case tess::TrianglesStrip:
      case tess::LineLoop:
        ASSERT(0, ("We've got invalid type during teselation:", disp.indices()[i].first));
      case tess::TrianglesList: break;
      }

      for (size_t j = 0; j < disp.indices()[i].second.size(); ++j)
      {
        int const idx = disp.indices()[i].second[j];
        tess::Vertex const & v = disp.vertices()[idx];
        triangles.push_back(m2::PointD(v.x, v.y));
      }

      ASSERT_EQUAL(triangles.size() % 3, 0, ());
    }
  }
}
