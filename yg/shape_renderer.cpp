#include "../base/SRC_FIRST.hpp"
#include "shape_renderer.hpp"
#include "skin.hpp"

namespace yg
{
  namespace gl
  {
    ShapeRenderer::ShapeRenderer(base_t::Params const & params) : base_t(params)
    {
    }

    void ShapeRenderer::drawArc(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointF> pts;
      approximateArc(center, startA, endA, r, pts);
      vector<m2::PointD> ptsD;
      copy(pts.begin(), pts.end(), back_inserter(ptsD));
      drawPath(&ptsD[0], ptsD.size(), skin()->mapPenInfo(yg::PenInfo(c, 3, 0, 0, 0)), depth);
    }

    void ShapeRenderer::approximateArc(m2::PointF const & center, double startA, double endA, double r, vector<m2::PointF> & pts)
    {
      double sectorA = math::pi / 180.0;
      unsigned sectorsCount = floor((endA - startA) / sectorA);
      sectorA = (endA - startA) / sectorsCount;

      for (unsigned i = 0; i <= sectorsCount; ++i)
        pts.push_back(m2::Shift(m2::Rotate(m2::PointD(r, 0), startA + i * sectorA), center));
    }

    void ShapeRenderer::drawSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointF> pts;

      pts.push_back(center);
      approximateArc(center, startA, endA, r, pts);
      pts.push_back(center);

      vector<m2::PointD> ptsD;
      copy(pts.begin(), pts.end(), back_inserter(ptsD));
      drawPath(&ptsD[0], ptsD.size(), skin()->mapPenInfo(yg::PenInfo(c, 2, 0, 0, 0)), depth);
    }

    void ShapeRenderer::fillSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointF> arcPts;

      arcPts.push_back(center);
      approximateArc(center, startA, endA, r, arcPts);

      m2::PointF pt0 = arcPts[0];
      m2::PointF pt1 = arcPts[1];

      vector<m2::PointD> sectorPts;

      for (size_t i = 2; i < arcPts.size(); ++i)
      {
        sectorPts.push_back(pt0);
        sectorPts.push_back(pt1);
        sectorPts.push_back(arcPts[i]);
        pt1 = arcPts[i];
      }

      drawTrianglesList(&sectorPts[0], sectorPts.size(), skin()->mapColor(c), depth);
    }

  }
}
