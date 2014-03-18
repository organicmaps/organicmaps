#include "shape_renderer.hpp"
#include "pen.hpp"
#include "brush.hpp"
#include "resource.hpp"
#include "resource_cache.hpp"

#include "opengl/base_texture.hpp"

#include "../geometry/point2d.hpp"

#include "../base/logging.hpp"

namespace graphics
{
  ShapeRenderer::ShapeRenderer(base_t::Params const & params) : base_t(params)
  {
  }

  void ShapeRenderer::drawConvexPolygon(m2::PointF const * pts, size_t ptsCount, graphics::Color const & color, double depth)
  {
    uint32_t resID = base_t::mapInfo(Brush::Info(color));

    if (resID == base_t::invalidHandle())
    {
      LOG(LDEBUG, ("cannot map color"));
      return;
    }

    drawTrianglesFan(pts, ptsCount, resID, depth);
  }

  void ShapeRenderer::drawArc(m2::PointD const & center, double startA, double endA, double r, graphics::Color const & c, double depth)
  {
    vector<m2::PointD> pts;
    approximateArc(center, startA, endA, r, pts);

    if (pts.size() < 2)
      return;

    drawPath(&pts[0],
             pts.size(),
             0,
             base_t::mapInfo(graphics::Pen::Info(c, 3, 0, 0, 0)),
             depth);
  }

  void ShapeRenderer::approximateArc(m2::PointD const & center, double startA, double endA, double r, vector<m2::PointD> & pts)
  {
    double sectorA = math::pi / 30.0;
    size_t const sectorsCount = max(size_t(1), static_cast<size_t>(floor(fabs(endA - startA) / sectorA)));
    sectorA = (endA - startA) / sectorsCount;

    for (size_t i = 0; i <= sectorsCount; ++i)
      pts.push_back(m2::Shift(m2::Rotate(m2::PointD(r, 0), startA + i * sectorA), center));
  }

  void ShapeRenderer::drawSector(m2::PointD const & center, double startA, double endA, double r, graphics::Color const & c, double depth)
  {
    vector<m2::PointD> pts;

    pts.push_back(center);
    approximateArc(center, startA, endA, r, pts);
    pts.push_back(center);

    if (pts.size() < 3)
      return;

    drawPath(&pts[0],
             pts.size(),
             0,
             base_t::mapInfo(graphics::Pen::Info(c, 2, 0, 0, 0)),
             depth);
  }

  void ShapeRenderer::fillSector(m2::PointD const & center, double startA, double endA, double r, graphics::Color const & c, double depth)
  {
    vector<m2::PointD> arcPts;

    arcPts.push_back(center);
    approximateArc(center, startA, endA, r, arcPts);

    if (arcPts.size() < 3)
      return;

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

    drawTrianglesList(&sectorPts[0],
                      sectorPts.size(),
                      base_t::mapInfo(Brush::Info(c)),
                      depth);
  }

  void ShapeRenderer::drawRectangle(m2::AnyRectD const & r, graphics::Color const & c, double depth)
  {
    uint32_t id = base_t::mapInfo(Brush::Info(c));
    Resource const * res = base_t::fromID(id);

    if (res == 0)
    {
      LOG(LDEBUG, ("cannot map color"));
      return;
    }

    m2::PointD rectPts[4];

    r.GetGlobalPoints(rectPts);
    swap(rectPts[2], rectPts[3]);

    m2::PointF rectPtsF[4];
    for (int i = 0; i < 4; ++i)
      rectPtsF[i] = m2::PointF(rectPts[i].x, rectPts[i].y);

    GeometryPipeline & p = pipeline(res->m_pipelineID);

    shared_ptr<gl::BaseTexture> texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    m2::PointF texPt = texture->mapPixel(m2::RectF(res->m_texRect).Center());

    m2::PointF normal(0, 0);

    addTexturedStripStrided(
          rectPtsF,
          sizeof(m2::PointF),
          &normal,
          0,
          &texPt,
          0,
          4,
          depth,
          res->m_pipelineID);
  }

  void ShapeRenderer::drawRectangle(m2::RectD const & r, graphics::Color const & c, double depth)
  {
    uint32_t id = base_t::mapInfo(Brush::Info(c));
    Resource const * res = base_t::fromID(id);

    if (res == 0)
    {
      LOG(LDEBUG, ("cannot map color"));
      return;
    }

    m2::PointF rectPts[4] = {
      m2::PointF(r.minX(), r.minY()),
      m2::PointF(r.maxX(), r.minY()),
      m2::PointF(r.minX(), r.maxY()),
      m2::PointF(r.maxX(), r.maxY())
    };

    GeometryPipeline & p = pipeline(res->m_pipelineID);

    shared_ptr<gl::BaseTexture> texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    m2::PointF texPt = texture->mapPixel(m2::RectF(res->m_texRect).Center());

    m2::PointF normal(0, 0);

    addTexturedStripStrided(
          rectPts,
          sizeof(m2::PointF),
          &normal,
          0,
          &texPt,
          0,
          4,
          depth,
          res->m_pipelineID
          );
  }

  void ShapeRenderer::drawRoundedRectangle(m2::RectD const & r, double rad, graphics::Color const & c, double depth)
  {
    uint32_t id = base_t::mapInfo(Brush::Info(c));
    Resource const * res = base_t::fromID(id);

    if (res == 0)
    {
      LOG(LDEBUG, ("cannot map color"));
      return;
    }

    GeometryPipeline & p = pipeline(res->m_pipelineID);

    shared_ptr<gl::BaseTexture> texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    m2::PointF texPt = texture->mapPixel(m2::RectF(res->m_texRect).Center());

    vector<m2::PointD> seg00;
    vector<m2::PointD> seg10;
    vector<m2::PointD> seg11;
    vector<m2::PointD> seg01;

    approximateArc(m2::PointD(r.minX() + rad, r.minY() + rad),
                   math::pi,
                   3 * math::pi / 2,
                   rad,
                   seg00);

    approximateArc(m2::PointD(r.minX() + rad, r.maxY() - rad),
                   math::pi / 2,
                   math::pi,
                   rad,
                   seg01);

    approximateArc(m2::PointD(r.maxX() - rad, r.maxY() - rad),
                   0,
                   math::pi / 2,
                   rad,
                   seg11);

    approximateArc(m2::PointD(r.maxX() - rad, r.minY() + rad),
                   3 * math::pi / 2,
                   math::pi * 2,
                   rad,
                   seg10);

    vector<m2::PointF> pts;

    for (unsigned i = 0; i < seg11.size(); ++i)
      pts.push_back(m2::PointF(seg11[i]));

    for (unsigned i = 0; i < seg01.size(); ++i)
      pts.push_back(m2::PointF(seg01[i]));

    for (unsigned i = 0; i < seg00.size(); ++i)
      pts.push_back(m2::PointF(seg00[i]));

    for (unsigned i = 0; i < seg10.size(); ++i)
      pts.push_back(m2::PointF(seg10[i]));

    m2::PointF normal(0, 0);

    addTexturedFanStrided(
          &pts[0],
          sizeof(m2::PointF),
          &normal,
          0,
          &texPt,
          0,
          pts.size(),
          depth,
          res->m_pipelineID
          );
  }
}
