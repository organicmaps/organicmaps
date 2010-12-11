#include "osm_element.hpp"

#include "../cell_id.hpp"

#include "../../3party/sgitess/interface.h"

namespace feature
{
  struct AddTessPointF
  {
    tess::Tesselator & m_tess;
    AddTessPointF(tess::Tesselator & tess) : m_tess(tess)
    {}
    void operator()(CoordPointT const & p)
    {
      m_tess.add(tess::Vertex(p.first, p.second));
    }
  };

  void TesselateInterior(FeatureBuilder & fb, feature::holes_cont_t const & holes)
  {
    ASSERT(fb.IsGeometryClosed(), ());

    tess::VectorDispatcher disp;
    tess::Tesselator tess;
    tess.setDispatcher(&disp);
    tess.setWindingRule(tess::WindingOdd);

    tess.beginPolygon();

    tess.beginContour();
    {
      vector<char> data;
      fb.Serialize(data);
      FeatureGeom f(data);
      f.ForEachPoint(AddTessPointF(tess));
    }
    tess.endContour();

    for (feature::holes_cont_t::const_iterator it = holes.begin(); it != holes.end(); ++it)
    {
      tess.beginContour();
      for (size_t i = 0; i < (*it).size(); ++i)
        tess.add(tess::Vertex((*it)[i].x, (*it)[i].y));
      tess.endContour();
    }

    tess.endPolygon();

    for (size_t i = 0; i < disp.indices().size(); ++i)
    {
      vector<m2::PointD> vertices;
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
        vertices.push_back(m2::PointD(v.x, v.y));
      }

      ASSERT_EQUAL(vertices.size() % 3, 0, ());
      size_t const triangleCount = vertices.size() / 3;
      for (size_t i = 0; i < triangleCount; ++i)
        fb.AddTriangle(vertices[3*i + 0], vertices[3*i + 1], vertices[3*i + 2]);
    }
  }
}
