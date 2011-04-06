#include "../base/SRC_FIRST.hpp"
#include "shape_renderer.hpp"
#include "skin.hpp"

#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    ShapeRenderer::ShapeRenderer(base_t::Params const & params) : base_t(params)
    {
    }

    void ShapeRenderer::drawArc(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointD> pts;
      approximateArc(center, startA, endA, r, pts);

      drawPath(&pts[0], pts.size(), skin()->mapPenInfo(yg::PenInfo(c, 3, 0, 0, 0)), depth);
    }

    void ShapeRenderer::approximateArc(m2::PointD const & center, double startA, double endA, double r, vector<m2::PointD> & pts)
    {
      double sectorA = math::pi / 180.0;
      size_t const sectorsCount = floor(fabs(endA - startA) / sectorA);
      sectorA = (endA - startA) / sectorsCount;

      for (size_t i = 0; i <= sectorsCount; ++i)
        pts.push_back(m2::Shift(m2::Rotate(m2::PointD(r, 0), startA + i * sectorA), center));
    }

    void ShapeRenderer::drawSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointD> pts;

      pts.push_back(center);
      approximateArc(center, startA, endA, r, pts);
      pts.push_back(center);

      drawPath(&pts[0], pts.size(), skin()->mapPenInfo(yg::PenInfo(c, 2, 0, 0, 0)), depth);
    }

    void ShapeRenderer::fillSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth)
    {
      vector<m2::PointD> arcPts;

      arcPts.push_back(center);
      approximateArc(center, startA, endA, r, arcPts);

      m2::PointD pt0 = arcPts[0];
      m2::PointD pt1 = arcPts[1];

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

    void ShapeRenderer::drawRectangle(m2::RectD const & r, yg::Color const & c, double depth)
    {
      ResourceStyle const * style = skin()->fromID(skin()->mapColor(c));

      if (style == 0)
      {
        LOG(LINFO, ("cannot map color"));
        return;
      }

      m2::PointF rectPts[4] = {
          m2::PointF(r.minX(), r.minY()),
          m2::PointF(r.maxX(), r.minY()),
          m2::PointF(r.minX(), r.maxY()),
          m2::PointD(r.maxX(), r.maxY())
        };

      m2::PointF texPt = skin()->pages()[style->m_pageID]->texture()->mapPixel(style->m_texRect.Center());

      addTexturedStripStrided(
          rectPts,
          sizeof(m2::PointF),
          &texPt,
          0,
          4,
          depth,
          style->m_pageID
          );
    }
  }
}
