#include "base/SRC_FIRST.hpp"
#include "3party/sgitess/interface.h"

#include "geometry/transformations.hpp"

#include "graphics/screen.hpp"
#include "graphics/pen.hpp"
#include "graphics/circle.hpp"
#include "graphics/brush.hpp"
#include "graphics/text_element.hpp"
#include "graphics/straight_text_element.hpp"
#include "graphics/path_text_element.hpp"

#include "graphics/opengl/utils.hpp"
#include "graphics/opengl/opengl.hpp"

#include "qt_tstfrm/macros.hpp"

#include "testing/testing.hpp"
#include <QtGui/QKeyEvent>

#include "base/math.hpp"
#include "base/string_utils.hpp"
#include "std/shared_ptr.hpp"

namespace
{
  struct TestDrawPoint
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawSymbol(m2::PointD(40, 40), 0, graphics::EPosCenter, 0);
      p->drawSymbol(m2::PointD(40.5, 60), 0, graphics::EPosCenter, 0);
      p->drawSymbol(m2::PointD(41, 80), 0, graphics::EPosCenter, 0);
    }
  };

  struct TestDrawLine
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      m2::PointD pts[3] =
      {
        m2::PointD(200, 200),
        m2::PointD(205, 200),
        m2::PointD(290, 200)
      };

//      double pat [] = {7, 7, 10, 10};
      double pat1 [] = {1, 1};
      p->drawPath(pts, 3, 0, p->mapInfo(graphics::Pen::Info(graphics::Color(0xFF, 0xFF, 0xFF, 0xFF), 2, pat1, 2, 0)), 0);
    }
  };

  struct TestDrawPathBase
  {
    std::vector<std::vector<m2::PointD> > m_pathes;
    std::vector<double> m_pathOffsets;
    //std::vector<std::vector<double> > m_patterns;
    std::vector<graphics::Pen::Info> m_penInfos;
    std::vector<double> m_depthes;

    std::vector<double> m_axisPattern;
    graphics::Pen::Info m_axisPenInfo;
    bool m_drawAxis;

    void Init()
    {
      m_drawAxis = false;

      m_axisPattern.push_back(2);
      m_axisPattern.push_back(2);
      m_axisPenInfo = graphics::Pen::Info(graphics::Color(0xFF, 0, 0, 0xFF), 2, &m_axisPattern[0], m_axisPattern.size(), 0, 0, 0, graphics::Pen::Info::ENoJoin, graphics::Pen::Info::EButtCap);
    }

    void AddTest(std::vector<m2::PointD> const & points,
        std::vector<double> const & pattern,
        graphics::Color const & color = graphics::Color(255, 255, 255, 255),
        double width = 2,
        double depth = 0,
        double pathOffset = 0,
        double penOffset = 0,
        graphics::Pen::Info::ELineCap lineCap = graphics::Pen::Info::ERoundCap,
        graphics::Pen::Info::ELineJoin lineJoin = graphics::Pen::Info::ERoundJoin
        )
    {
      m_pathes.push_back(points);
      m_pathOffsets.push_back(pathOffset);
      //m_patterns.push_back(pattern);
      m_penInfos.push_back(graphics::Pen::Info(color, width, pattern.empty() ? 0 : &pattern[0], pattern.size(), penOffset, 0, 0, lineJoin, lineCap));
      m_depthes.push_back(depth);
    }

    void AddTest(vector<m2::PointD> const & pts,
                 graphics::Pen::Info const & info,
                 double depth,
                 double pathOffset)
    {
      m_pathes.push_back(pts);
      m_pathOffsets.push_back(pathOffset);
      m_depthes.push_back(depth);
      m_penInfos.push_back(info);
    }

    std::vector<m2::PointD> & GetTestPoints(size_t i)
    {
      return m_pathes[i];
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      for (size_t i = 0; i < m_pathes.size(); ++i)
      {
        p->drawPath(&m_pathes[i][0],
                    m_pathes[i].size(),
                    m_pathOffsets[i],
                    p->mapInfo(m_penInfos[i]),
                    m_depthes[i]);
        if (m_drawAxis)
          p->drawPath(&m_pathes[i][0],
                      m_pathes[i].size(),
                      0,
                      p->mapInfo(m_axisPenInfo),
                      m_depthes[i]);
      }
    }

    void makeStar(vector<m2::PointD> & pts, m2::RectD const & r)
    {
      pts.push_back(m2::PointD(r.minX(), r.maxY()));
      pts.push_back(m2::PointD(r.Center().x, r.minY()));
      pts.push_back(m2::PointD(r.maxX(), r.maxY()));
      pts.push_back(m2::PointD(r.minX(), r.minY() + r.SizeY() / 3));
      pts.push_back(m2::PointD(r.maxX(), r.minY() + r.SizeY() / 3));
      pts.push_back(m2::PointD(r.minX(), r.maxY()));
    }

  };

  struct TestDrawPathWithResourceCacheMiss : public TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();
      m_drawAxis = false;

      vector<m2::PointD> points;
      vector<double> pattern;

      size_t const columns = 30;
      size_t const rows = 6;

      for (size_t j = 0; j < rows; ++j)
      {
        for (size_t i = 0; i < columns; ++i)
        {
          points.clear();
          points.push_back(m2::PointD(100 * j + 10, i * 15 + 20));
          points.push_back(m2::PointD(100 * j + 100, i * 15 + 20));
          AddTest(points, pattern, graphics::Color(128 + 128 / columns * i, 128 + 128 / rows * j, 0, 255), rand() % 15);
        }
      }
    }
  };

  struct TestDrawPathWithOffset : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      m_drawAxis = true;

      vector<m2::PointD> pts;
      vector<double> pattern;

      pattern.push_back(20);
      pattern.push_back(30);

      pts.push_back(m2::PointD(200, 80));
      pts.push_back(m2::PointD(300, 80));

      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 150);

      pts.clear();
      pts.push_back(m2::PointD(200, 90));
      pts.push_back(m2::PointD(300, 90));

      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, -150);

      pts.clear();
      pts.push_back(m2::PointD(200, 100));
      pts.push_back(m2::PointD(400, 100));


      /// The path should start from -10px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, -10);

      pts.clear();
      pts.push_back(m2::PointD(200, 110));
      pts.push_back(m2::PointD(400, 110));

      /// The path should start from 0px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 0);

      pts.clear();
      pts.push_back(m2::PointD(200, 120));
      pts.push_back(m2::PointD(400, 120));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 60);

      pts.clear();
      pts.push_back(m2::PointD(200, 130));
      pts.push_back(m2::PointD(400, 130));

      /// The path should start from 0px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 0, -10);

      pts.clear();
      pts.push_back(m2::PointD(200, 140));
      pts.push_back(m2::PointD(400, 140));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 0, 0);

      pts.clear();
      pts.push_back(m2::PointD(200, 150));
      pts.push_back(m2::PointD(400, 150));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, graphics::Color(0, 0, 0, 255), 4, 0, 0, 60);

    }
  };

  struct TestDrawPathJoin : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;
    bool OnKeyPress(QKeyEvent * kev)
    {
      if (kev->key() == Qt::Key_Up)
      {
        GetTestPoints(0).back().y--;
        return true;
      }
      if (kev->key() == Qt::Key_Down)
      {
        GetTestPoints(0).back().y++;
        return true;
      }
      if (kev->key() == Qt::Key_Left)
      {
        GetTestPoints(0).back().x--;
        return true;
      }
      if (kev->key() == Qt::Key_Right)
      {
        GetTestPoints(0).back().x++;
        return true;
      }

      return false;
    }

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.push_back(m2::PointD(220, 300));
      testPoints.push_back(m2::PointD(280, 240));
      testPoints.push_back(m2::PointD(320, 369));
      testPoints.push_back(m2::PointD(420, 370));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 40);

      testPoints.clear();
      testPoints.push_back(m2::PointD(320, 300));
      testPoints.push_back(m2::PointD(380, 240));
      testPoints.push_back(m2::PointD(420, 369));
      testPoints.push_back(m2::PointD(520, 370));

      AddTest(testPoints, testPattern, graphics::Color(0, 255, 0, 255), 40);

//      testPoints.clear();
//
//      testPoints.push_back(m2::PointD(460, 100));
//      testPoints.push_back(m2::PointD(505, 200));
//      testPoints.push_back(m2::PointD(600, 150));
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 255, 255), 40);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 120));
//      testPoints.push_back(m2::PointD(160, 120));
//
//      testPattern.clear();
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 200));
//      testPoints.push_back(m2::PointD(50, 350));
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 120));
//      testPoints.push_back(m2::PointD(250, 120));
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 200));
//      testPoints.push_back(m2::PointD(400, 350));
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(100, 200));
//      testPoints.push_back(m2::PointD(240, 220));
//
//      AddTest(testPoints, testPattern, graphics::Color(0, 0, 0, 255), 60);
    }
  };

  struct TestDrawPathSolidWithZ : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(200, 200));
      testPoints.push_back(m2::PointD(300, 200));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 40, 0.5);

//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(150, 220));
      testPoints.push_back(m2::PointD(300, 220));

      AddTest(testPoints, testPattern, graphics::Color(0, 255, 0, 255), 40, 0.5);

      testPattern.clear();
      testPoints.clear();

      testPoints.push_back(m2::PointD(200, 240));
      testPoints.push_back(m2::PointD(300, 240));

      AddTest(testPoints, testPattern, graphics::Color(0, 0, 255, 255), 40, 0);
    }
  };

  struct TestDrawPathSolidWithClipRect : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(200, 200));
      testPoints.push_back(m2::PointD(0, 0));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 40, 0.5);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->enableClipRect(true);
      p->setClipRect(m2::RectI(50, 70, 150, 150));
      base_t::DoDraw(p);
      p->endFrame();
      m2::RectI r(0, 0, 0, 0);
      p->beginFrame();
      p->setClipRect(r);
      p->enableClipRect(true);
      p->clear(graphics::Screen::s_bgColor);
    }
  };

  struct TestDrawPathSolidDiffWidth : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      vector<m2::PointD> testPoints;

      int starCount = 10;
      m2::PointD starSize(50, 50);
      m2::PointD pt(10, 10);

      for (int i = 0; i < starCount; ++i)
      {
        base_t::makeStar(testPoints, m2::RectD(pt, pt + starSize));
        AddTest(testPoints, vector<double>(), graphics::Color(255, 0, 0, 255), i + 1);
        pt = pt + m2::PointD(starSize.x + i + 3, 0);
        testPoints.clear();
      }

      pt = m2::PointD(10, 10 + starSize.y + 10);

      vector<double> pat;
      pat.push_back(20);
      pat.push_back(5);

      for (int i = 0; i < starCount; ++i)
      {
        base_t::makeStar(testPoints, m2::RectD(pt, pt + starSize));
        AddTest(testPoints, pat, graphics::Color(255, 0, 0, 255), i + 1);
        pt = pt + m2::PointD(starSize.x + i + 3, 0);
        testPoints.clear();
      }
    }
  };

  struct TestDrawPathZigZag : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      vector<m2::PointD> testPoints;

      testPoints.push_back(m2::PointD(20, 100));
      testPoints.push_back(m2::PointD(50, 20));
      testPoints.push_back(m2::PointD(80, 100));
      testPoints.push_back(m2::PointD(110, 20));

      AddTest(testPoints, vector<double>(), graphics::Color(255, 0, 0, 255), 6);

      for (unsigned i = 0; i < testPoints.size(); ++i)
        testPoints[i].y += 100;

      vector<double> pat;
      pat.push_back(10);
      pat.push_back(10);

      AddTest(testPoints, pat, graphics::Color(255, 0, 0, 255), 6);
    }
  };


  struct TestDrawPathSolid1PX : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(120, 200));
      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 1);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 1);
    }
  };

  struct TestDrawPathSolid2PX : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(120, 200));
      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 2);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, graphics::Color(255, 0, 0, 255), 2);
    }
  };


  struct TestDrawPathSolid : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();
      m_drawAxis = true;

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      graphics::Pen::Info::ELineJoin joins[] = {
        graphics::Pen::Info::ERoundJoin,
        graphics::Pen::Info::EBevelJoin,
        graphics::Pen::Info::ENoJoin
      };

      graphics::Pen::Info::ELineCap caps[] = {
        graphics::Pen::Info::EButtCap,
        graphics::Pen::Info::ESquareCap,
        graphics::Pen::Info::ERoundCap
      };

      double dx = 0, dy = 0, width = 30;
      for (int dashed = 0; dashed < 2; ++dashed)
      {
        for (int join = 0; join < ARRAY_SIZE(joins); join++)
        {
          for (int cap = 0; cap < ARRAY_SIZE(caps); cap++)
          {

            testPoints.clear();
            testPoints.push_back(m2::PointD(dx + 20, dy + 100));
            testPoints.push_back(m2::PointD(dx + 80, dy + 40));
            testPoints.push_back(m2::PointD(dx + 120, dy + 169));
            testPoints.push_back(m2::PointD(dx + 220, dy + 170));
            AddTest(testPoints, testPattern, graphics::Color(0, 255, 0, 255), width, 0, 0, 0, caps[cap], joins[join]);

            dy += 130;
          }
          dx += 200;
          dy = 0;
        }
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(10);
      testPattern.push_back(20);
      }
    }
  };

  struct TestDrawPathSymbol : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      m_drawAxis = true;

      vector<m2::PointD> pts;

      graphics::Pen::Info info;
      graphics::Pen::Info info1;

      info.m_icon.m_name = "theatre";
      info.m_step = 15;

      pts.clear();
      pts.push_back(m2::PointD(100, 100));
      pts.push_back(m2::PointD(200, 100));

      AddTest(pts, info, graphics::maxDepth, 0);

      pts.clear();
      pts.push_back(m2::PointD(100, 200));
      pts.push_back(m2::PointD(200, 200));

      AddTest(pts, info, graphics::maxDepth, 30);

      pts.clear();
      pts.push_back(m2::PointD(100, 300));
      pts.push_back(m2::PointD(200, 300));

      vector<double> pat;

      pat.push_back(20);
      pat.push_back(20);

      info1 = graphics::Pen::Info(graphics::Color(0, 0, 0, 255), 20, &pat[0], pat.size());
      AddTest(pts, info1, graphics::maxDepth, 30);

      pts.clear();
      pts.push_back(m2::PointD(300, 100));
      pts.push_back(m2::PointD(400, 160));
      pts.push_back(m2::PointD(300, 240));

      AddTest(pts, info, graphics::maxDepth, 0);
    }
  };

  struct TestDrawPoly
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
//      m2::PointD ptsStrip[5] = {m2::PointD(10, 10), m2::PointD(40, 10), m2::PointD(70, 10), m2::PointD(90, 60), m2::PointD(130, 30)};
//      p->drawTriangles(ptsStrip, 5, graphics::TriangleStrip, p->mapColor(graphics::Color(255, 0, 0, 255)));

//      m2::PointD ptsFan[5] = {m2::PointD(150, 20), m2::PointD(170, 80), m2::PointD(190, 100), m2::PointD(200, 80), m2::PointD(220, 60)};
//      p->drawTriangles(ptsFan, 5, graphics::TriangleFan, p->mapColor(graphics::Color(0, 255, 0, 255)));

      m2::PointD ptsList[6] = {m2::PointD(20, 80), m2::PointD(50, 120), m2::PointD(80, 80), m2::PointD(110, 80), m2::PointD(140, 120), m2::PointD(80, 120)};
      p->drawTrianglesList(ptsList, 6, /*graphics::TriangleList, */p->mapInfo(graphics::Brush::Info(graphics::Color(0, 0, 255, 255))), 0);
    }
  };

  /// Trying to draw polygon with more vertices that fits into internal buffer.
  struct TestDrawPolyOverflow
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      size_t verticesCount = 30000;
      vector<m2::PointD> vertices;

      double r = 200;
      m2::PointD offsetPt(300, 300);

      m2::PointD centerPt(0, 0);
      m2::PointD prevPt(r, 0);

      double const Angle = 2 * 3.1415 / verticesCount;
      double const sinA = sin(Angle);
      double const cosA = cos(Angle);

      for (size_t i = 0; i < verticesCount; ++i)
      {
        vertices.push_back(centerPt + offsetPt);

        m2::PointD nextPt(prevPt.x * cosA + prevPt.y * sinA, -prevPt.x * sinA + prevPt.y * cosA);

        vertices.push_back(prevPt + offsetPt);
        vertices.push_back(nextPt + offsetPt);

        prevPt = nextPt;
      }

      p->drawTrianglesList(&vertices[0],
                           vertices.size(),
                           p->mapInfo(graphics::Brush::Info(graphics::Color(0, 0, 255, 255))), 0);
    }
  };

  struct TestDrawText
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      m2::PointD path[2] = {m2::PointD(100, 200), m2::PointD(1000, 200)};
      double pat[2] = {2, 2};
      p->drawPath(path,
                  sizeof(path) / sizeof(m2::PointD),
                  0,
                  p->mapInfo(graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, pat, 2, 0)),
                  0);

      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));

      p->drawText(fontDesc, m2::PointD(200, 200), graphics::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(240, 200), graphics::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(280, 200), graphics::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(320, 200), graphics::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(360, 200), graphics::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  struct TestDrawSingleSymbol
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, "X", 1, true);
    }
  };

  struct TestDrawEmptySymbol
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, " ", 1, true);
    }
  };

  struct TestDrawStringOnString
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      size_t const maxTimes = 10;
      size_t const yStep = 30;

      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));

      for (size_t i = 0; i < maxTimes; ++i)
        for (size_t j = 1; j <= i+1; ++j)
          p->drawText(fontDesc, m2::PointD(40, 10 + yStep * i), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  struct TestDrawSingleSymbolAndSolidPath
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      vector<m2::PointD> path;
      path.push_back(m2::PointD(40, 50));
      path.push_back(m2::PointD(70, 50));

      double pat[] = { 2, 2 };
      graphics::Pen::Info penInfo = graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
      graphics::Pen::Info solidPenInfo = graphics::Pen::Info(graphics::Color(0xFF, 0, 0, 0xFF), 4, 0, 0, 0);

      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));

      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, "S", 0, true);
      p->drawPath(&path[0],
                  path.size(),
                  0,
                  p->mapInfo(solidPenInfo),
                  0);
    }
  };

  struct TestDrawMultiLineStringWithPosition
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(14, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));

      m2::PointD pt = m2::PointD(50, 150);

      p->drawText(fontDesc, pt, graphics::EPosAboveRight, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(50, 300);

      p->drawText(fontDesc, pt, graphics::EPosRight, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(50, 450);

      p->drawText(fontDesc, pt, graphics::EPosUnderRight, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(400, 150);

      p->drawText(fontDesc, pt, graphics::EPosAbove, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(400, 300);

      p->drawText(fontDesc, pt, graphics::EPosCenter, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(400, 450);

      p->drawText(fontDesc, pt, graphics::EPosUnder, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(750, 150);

      p->drawText(fontDesc, pt, graphics::EPosAboveLeft, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(750, 300);

      p->drawText(fontDesc, pt, graphics::EPosLeft, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);

      pt = m2::PointD(750, 450);

      p->drawText(fontDesc, pt, graphics::EPosUnderLeft, "Simplicity is the ultimate sophistication", graphics::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), graphics::Color(0, 0, 0, 255), graphics::maxDepth);
    }
  };

  struct TestDrawString
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 150), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  double calc_length(m2::PointD const * v, size_t s)
  {
    double ret = 0.0;
    for (size_t i = 1; i < s; ++i)
      ret += v[i-1].Length(v[i]);
    return ret;
  }

  double calc_length(vector<m2::PointD> const & v)
  {
    return calc_length(&v[0], v.size());
  }

  struct TestDrawThaiString
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      string s("ถนนสุขุมวิท");

      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 150), graphics::EPosAboveRight, s, 0, true);

      m2::PointD pts[2] = {m2::PointD(200, 100), m2::PointD(400, 300)};

      p->drawPathText(fontDesc, pts, ARRAY_SIZE(pts), s, calc_length(pts, ARRAY_SIZE(pts)), 0, graphics::EPosCenter, graphics::maxDepth);
    }
  };

  struct TestDrawStringWithColor
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(25, graphics::Color(0, 0, 255, 255), true, graphics::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };


  struct TestDrawUnicodeSymbols
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::FontDesc fontDesc(12);
      p->drawText(fontDesc, m2::PointD(40, 50), graphics::EPosAboveRight, "Latin Symbol : A", 0, true);
      p->drawText(fontDesc, m2::PointD(40, 80), graphics::EPosAboveRight, "Cyrillic Symbol : Ы", 0, true);
    }
  };

  struct TestDrawTextRect : TestDrawString
  {
    typedef TestDrawString base_t;
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      m2::PointD startPt(40, 50);

      graphics::StraightTextElement::Params params;
      params.m_depth = 0;
      params.m_fontDesc = graphics::FontDesc(20, graphics::Color(0, 0, 0, 0), true, graphics::Color(255, 255, 255, 255));
      params.m_log2vis = false;
      params.m_pivot = startPt;
      params.m_position = graphics::EPosAboveRight;
      params.m_glyphCache = p->glyphCache();
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication");
      graphics::StraightTextElement ste(params);

      m2::RectD const r = ste.GetBoundRect();
      p->drawRectangle(r, graphics::Color(0, 0, 255, 255), 0);

      base_t::DoDraw(p);
    }
  };


  struct TestDrawTextOnPathBigSymbols
  {
    vector<m2::PointD> m_path;
    string m_text;
    graphics::Pen::Info m_penInfo;

    void Init()
    {
      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(80, 200));
      m_path.push_back(m2::PointD(180, 250));

      m_text = "Syp";

      double pat[2] = {2, 2};
      m_penInfo = graphics::Pen::Info(graphics::Color(0xFF, 0xFF, 0xFF, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawPath(&m_path[0],
                  m_path.size(),
                  0,
                  p->mapInfo(m_penInfo),
                  1);

      graphics::FontDesc fontDesc(30);

      p->drawPathText(fontDesc,
                      &m_path[0],
                      m_path.size(),
                      m_text,
                      calc_length(m_path),
                      0.0,
                      0,
                      0);
    }
  };

  struct TestDrawTextOnPathInteractive
  {
    double m_pathOffset;
    vector<m2::PointD> m_testPoints;
    string m_text;

    bool OnKeyPress(QKeyEvent * kev)
    {
      if (kev->key() == Qt::Key_Left)
      {
        m_pathOffset += 1;
        return true;
      }
      if (kev->key() == Qt::Key_Right)
      {
        m_pathOffset -= 1;
        return true;
      }

      return false;
    }

    void Init()
    {
      m_pathOffset = -102;
      //m_pathOffset = 0;
      m_testPoints.push_back(m2::PointD(40, 200));
      m_testPoints.push_back(m2::PointD(100, 100));
      m_testPoints.push_back(m2::PointD(160, 200));
      m_testPoints.push_back(m2::PointD(200, 100));
      m_testPoints.push_back(m2::PointD(240, 200));
      m_testPoints.push_back(m2::PointD(280, 100));
      m_testPoints.push_back(m2::PointD(320, 200));
      m_testPoints.push_back(m2::PointD(360, 100));
      m_testPoints.push_back(m2::PointD(400, 200));

    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawPath(&m_testPoints[0], m_testPoints.size(), 0, p->mapInfo(graphics::Pen::Info(graphics::Color(255, 255, 255, 255), 2, 0, 0, 0)), 0);
      graphics::FontDesc fontDesc(20, graphics::Color(0, 0, 0, 255), false);
      //m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";
      m_text = "Vinci";
      p->drawPathText(fontDesc, &m_testPoints[0], m_testPoints.size(), m_text.c_str(), calc_length(m_testPoints), m_pathOffset, graphics::EPosLeft, 1);
    }
  };

  struct TestDrawTextOnPath
  {
    std::vector<m2::PointD> m_path;
    std::string m_text;
    graphics::Pen::Info m_penInfo;

    TestDrawTextOnPath()
    {

      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(100, 100));
      m_path.push_back(m2::PointD(600, 100));
      m_path.push_back(m2::PointD(400, 300));
      m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";

      double pat[] = { 2, 2 };
      m_penInfo = graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), 0, p->mapInfo(m_penInfo), 0);
      graphics::FontDesc fontDesc(20);
      p->drawPathText(fontDesc, &m_path[0], m_path.size(), m_text, calc_length(m_path), 0.0, graphics::EPosCenter, 0);
    }
  };

  struct TestDrawStraightTextElement
  {
    graphics::Pen::Info m_penInfo;
    vector<m2::PointD> m_path;
    TestDrawStraightTextElement()
    {
      m_path.push_back(m2::PointD(100, 200));
      m_path.push_back(m2::PointD(500, 200));
      double pat[] = { 2, 2 };
      m_penInfo = graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::StraightTextElement::Params params;
      params.m_fontDesc = graphics::FontDesc(20);
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication. Leonardo Da Vinci.");
      params.m_depth = 10;
      params.m_log2vis = false;
      params.m_glyphCache = p->glyphCache();
      params.m_pivot = m_path[0];
      params.m_position = graphics::EPosRight;

      graphics::StraightTextElement ste(params);

      p->drawPath(&m_path[0], m_path.size(), 0, p->mapInfo(m_penInfo), 0);
      ste.draw(p.get(), math::Identity<double, 3>());
    }
  };

  struct TestDrawPathTextElement
  {
    vector<m2::PointD> m_path;
    graphics::Pen::Info m_penInfo;

    TestDrawPathTextElement()
    {
      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(100, 100));
      m_path.push_back(m2::PointD(160, 200));
      m_path.push_back(m2::PointD(200, 100));
      m_path.push_back(m2::PointD(240, 200));
      m_path.push_back(m2::PointD(280, 100));
      m_path.push_back(m2::PointD(320, 200));
      m_path.push_back(m2::PointD(360, 100));
      m_path.push_back(m2::PointD(400, 200));

      double pat[] = { 2, 2 };
      m_penInfo = graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      graphics::PathTextElement::Params params;
      params.m_pts = &m_path[0];
      params.m_ptsCount = m_path.size();
      params.m_fullLength = calc_length(m_path);
      params.m_pathOffset = 0;
      params.m_fontDesc = graphics::FontDesc(20);
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication. Leonardo Da Vinci.");
      params.m_depth = 10;
      params.m_log2vis = false;
      params.m_glyphCache = p->glyphCache();
      params.m_pivot = m_path[0];
      params.m_position = graphics::EPosCenter;
      params.m_textOffset = 30;

      graphics::PathTextElement pte(params);

      p->drawPath(&m_path[0], m_path.size(), 0, p->mapInfo(m_penInfo), 0);
      pte.draw(p.get(), math::Identity<double, 3>());
    }
  };

  struct TestDrawTextOnPathZigZag
  {
    std::vector<m2::PointD> m_path;
    std::string m_text;
    graphics::Pen::Info m_penInfo;

    TestDrawTextOnPathZigZag()
    {
      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(100, 100));
      m_path.push_back(m2::PointD(160, 200));
      m_path.push_back(m2::PointD(200, 100));
      m_path.push_back(m2::PointD(240, 200));
      m_path.push_back(m2::PointD(280, 100));
      m_path.push_back(m2::PointD(320, 200));
      m_path.push_back(m2::PointD(360, 100));
      m_path.push_back(m2::PointD(400, 200));
      m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";

      double pat[] = { 2, 2 };
      m_penInfo = graphics::Pen::Info(graphics::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), 0, p->mapInfo(m_penInfo), 0);
//      graphics::FontDesc fontDesc(false, 10);
      graphics::FontDesc fontDesc(20);
      p->drawPathText(fontDesc, &m_path[0], m_path.size(), m_text, calc_length(m_path), 0.0, graphics::EPosCenter, 0);
    }
  };

  struct TestDrawTextOnPathWithOffset : TestDrawTextOnPath
  {
    vector<m2::PointD> m_pathUnder;
    vector<m2::PointD> m_pathAbove;

    TestDrawTextOnPathWithOffset()
    {
      copy(m_path.begin(), m_path.end(), back_inserter(m_pathUnder));
      for (size_t i = 0; i < m_pathUnder.size(); ++i)
        m_pathUnder[i].y -= 50;

      std::copy(m_path.begin(), m_path.end(), back_inserter(m_pathAbove));
      for (size_t i = 0; i < m_pathUnder.size(); ++i)
        m_pathAbove[i].y += 50;
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      TestDrawTextOnPath::DoDraw(p);

      p->drawPath(&m_pathAbove[0], m_pathAbove.size(), 0, p->mapInfo(m_penInfo), 0);
      p->drawPath(&m_pathUnder[0], m_pathUnder.size(), 0, p->mapInfo(m_penInfo), 0);

      double const len = calc_length(m_path);
      graphics::FontDesc fontDesc(20);

      p->drawPathText(fontDesc, &m_pathAbove[0], m_pathAbove.size(), m_text, len, 0.0, graphics::EPosAbove, 0);
      p->drawPathText(fontDesc, &m_pathUnder[0], m_pathUnder.size(), m_text, len, 0.0, graphics::EPosUnder, 0);
    }
  };

  struct TestDrawTextOverflow
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        graphics::FontDesc fontDesc(startSize + i);
        p->drawText(fontDesc, m2::PointD(10, startY), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication. Leonardo Da Vinci", 0,  true);
        startY += fontDesc.m_size;
      }
    }
  };

  struct TestDrawTextFiltering
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        graphics::FontDesc fontDesc(startSize);
        p->drawText(fontDesc, m2::PointD(10, startY), graphics::EPosAboveRight, "Simplicity is the ultimate sophistication. Leonardo Da Vinci", 100, true);
        p->drawText(fontDesc, m2::PointD(5, startY + (startSize + i) / 2), graphics::EPosAboveRight, "This text should be filtered", 100, true);
        startY += startSize + i;
      }
    }
  };

  struct TestDrawRandomTextFiltering
  {
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      char const * texts [] = {"Simplicity is the ultimate sophistication", "Leonardo Da Vinci"};

      int startSize = 20;
      int endSize = 40;

      int textsCount = 200;

      for (int i = 0; i < textsCount; ++i)
      {
        graphics::FontDesc fontDesc(rand() % (endSize - startSize) + startSize,
                              graphics::Color(rand() % 255, rand() % 255, rand() % 255, 255)
                              );
        p->drawText(
              fontDesc,
              m2::PointD(rand() % 500, rand() % 500),
              graphics::EPosAboveRight,
              texts[rand() % (sizeof(texts) / sizeof(char*))],
              rand() % 10,
              true);
      }
    }
  };

  struct TestDrawAnyRect
  {
  public:
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      m2::AnyRectD r[3] =
      {
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, 0, 50, 20)),
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, -10, 50, 10)),
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, -22, 50, -2))
      };

      p->drawRectangle(r[0], graphics::Color(255, 0, 0, 128), graphics::maxDepth - 2);
      if (!r[0].IsIntersect(r[1]))
        p->drawRectangle(r[1], graphics::Color(0, 255, 0, 128), graphics::maxDepth - 1);
      if (!r[0].IsIntersect(r[2]))
        p->drawRectangle(r[1], graphics::Color(0, 0, 255, 128), graphics::maxDepth);
    }
  };

  struct TestDrawSector
  {
  public:
    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      p->drawArc(m2::PointD(100, 100), 0, math::pi * 2, 30, graphics::Color(0, 0, 255, 128), 12000);
      p->fillSector(m2::PointD(100, 100), 0, math::pi * 2, 30, graphics::Color(0, 0, 255, 64), 12000);
    }
  };

  struct TestDrawSGIConvex
  {
    tess::VectorDispatcher m_d;
    std::vector<tess::Vertex> m_vertices;
    TestDrawSGIConvex()
    {
      m_vertices.push_back(tess::Vertex(100, 100));
      m_vertices.push_back(tess::Vertex(300, 100));
      m_vertices.push_back(tess::Vertex(250, 120));
      m_vertices.push_back(tess::Vertex(300, 300));
      m_vertices.push_back(tess::Vertex(100, 300));
      m_vertices.push_back(tess::Vertex(180, 250));
      m_vertices.push_back(tess::Vertex(120, 200));
      m_vertices.push_back(tess::Vertex(60, 150));
      m_vertices.push_back(tess::Vertex(100, 100));

      tess::Tesselator t;
      t.setDispatcher(&m_d);
      t.setBoundaryOnly(false);
      t.setWindingRule(tess::WindingNonZero);
      t.beginPolygon();
      t.beginContour();
      for (size_t i = 0; i < m_vertices.size(); ++i)
        t.add(m_vertices[i]);
      t.endContour();
      t.endPolygon();
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      double inputDataPat[] = {10, 0};
      graphics::Pen::Info inputDataRule(graphics::Color::fromARGB(0xFF000000), 6, inputDataPat, 2, 0);

      double triangleFanPat[] = {10, 10};
      graphics::Pen::Info triangleFanRule(graphics::Color::fromARGB(0xFFFF0000), 5, triangleFanPat, 2, 0);

      double triangleStripPat[] = {10, 10};
      graphics::Pen::Info triangleStripRule(graphics::Color::fromARGB(0xFF00FF00), 4, triangleStripPat, 2, 0);

      double triangleListPat[] = {10, 10};
      graphics::Pen::Info triangleListRule(graphics::Color::fromARGB(0xFF0000FF), 3, triangleListPat, 2, 0);

      double lineLoopPat[] = {2, 2};
      graphics::Pen::Info lineLoopRule(graphics::Color::fromARGB(0xFF00FFFF), 2, lineLoopPat, 2, 0);

      uint32_t inputDataID = p->mapInfo(inputDataRule);
      uint32_t triangleFanID = p->mapInfo(triangleFanRule);
      /*uint32_t triangleStripID = */p->mapInfo(triangleStripRule);
      uint32_t triangleListID = p->mapInfo(triangleListRule);
      uint32_t lineLoopID = p->mapInfo(lineLoopRule);

      p->drawPath((m2::PointD const *)&m_vertices[0], m_vertices.size(), 0, inputDataID, 0);

      for (size_t i = 0; i < m_d.indices().size(); ++i)
      {
        std::vector<std::vector<m2::PointD > > poly;

        switch (m_d.indices()[i].first)
        {
        case tess::TrianglesFan:
          {
            for (size_t j = 2; j < m_d.indices()[i].second.size(); ++j)
            {
              poly.push_back(std::vector<m2::PointD>());
              int first = m_d.indices()[i].second[0];
              int second = m_d.indices()[i].second[1];
              int third = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), 0, triangleFanID, 0);
            break;
          }
        case tess::TrianglesList:
          {
            for (size_t j = 0; j < m_d.indices()[i].second.size() / 3; ++j)
            {
              poly.push_back(std::vector<m2::PointD>());

              int first = m_d.indices()[i].second[j * 3];
              int second = m_d.indices()[i].second[j * 3 + 1];
              int third = m_d.indices()[i].second[j * 3 + 2];

              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), 0, triangleListID, 0);
            break;
          }
        case tess::TrianglesStrip:
          {
            for (size_t j = 2; j < m_d.indices()[i].second.size(); ++j)
            {
              poly.push_back(std::vector<m2::PointD>());
              int first = m_d.indices()[i].second[j - 2];
              int second = m_d.indices()[i].second[j - 1];
              int third = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), 0, triangleFanID, 0);
            break;
          }
        case tess::LineLoop:
          {
            poly.push_back(std::vector<m2::PointD>());
            for (size_t j = 0; j < m_d.indices()[i].second.size(); ++j)
            {
              int idx = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[idx].x, m_d.vertices()[idx].y));
            }

            poly.back().push_back(poly.back()[0]);

            p->drawPath(&poly[0][0], poly[0].size(), 0, lineLoopID, 0);
            break;
          }
        }
      }
    }
  };

  struct TestDrawSymbolFiltering
  {
    void DoDraw(shared_ptr<graphics::Screen> const & p)
    {
      for (int i = 0; i < 40; ++i)
        p->drawSymbol(m2::PointD(100 + i, 100), "hospital", graphics::EPosCenter, 0);
    }
  };

  struct TestDrawCircle
  {
    void DoDraw(shared_ptr<graphics::Screen> const & p)
    {
      graphics::Circle::Info ci0(10, graphics::Color(255, 0, 0, 255));
      p->drawCircle(m2::PointD(200, 200), ci0, graphics::EPosCenter, 100);

      graphics::Circle::Info ci1(10, graphics::Color(255, 0, 0, 255), true, 4, graphics::Color(255, 255, 255, 255));
      p->drawCircle(m2::PointD(100, 200), ci1, graphics::EPosCenter, 100);
    }
  };

  struct TestDrawImage
  {
    void DoDraw(shared_ptr<graphics::Screen> const & p)
    {
      graphics::Image::Info ii("test.png", graphics::EDensityMDPI);

      math::Matrix<double, 3, 3> m =
          math::Shift(
            math::Rotate(
              math::Identity<double, 3>(),
              math::pi / 4),
            100, 100);

      p->drawImage(m,
                   p->mapInfo(ii),
                   graphics::maxDepth);
    }
  };

   //UNIT_TEST_GL(TestDrawPolyOverflow);
   //UNIT_TEST_GL(TestDrawText);
   //UNIT_TEST_GL(TestDrawSingleSymbol);
   //UNIT_TEST_GL(TestDrawEmptySymbol);
   //UNIT_TEST_GL(TestDrawSingleSymbolAndSolidPath);
   //UNIT_TEST_GL(TestDrawMultiLineStringWithPosition);
   //UNIT_TEST_GL(TestDrawString);
   //UNIT_TEST_GL(TestDrawThaiString);
   //UNIT_TEST_GL(TestDrawStringWithColor);
   //UNIT_TEST_GL(TestDrawUnicodeSymbols);
   //UNIT_TEST_GL(TestDrawTextRect);
   //UNIT_TEST_GL(TestDrawStringOnString);
   //UNIT_TEST_GL(TestDrawTextOnPathInteractive);
   //UNIT_TEST_GL(TestDrawTextOnPathBigSymbols);
   //UNIT_TEST_GL(TestDrawTextOnPath);
   //UNIT_TEST_GL(TestDrawTextOnPathZigZag);
   //UNIT_TEST_GL(TestDrawTextOnPathWithOffset);
   //UNIT_TEST_GL(TestDrawStraightTextElement);
   //UNIT_TEST_GL(TestDrawPathTextElement);
   //UNIT_TEST_GL(TestDrawTextOverflow);
   //UNIT_TEST_GL(TestDrawTextFiltering);
   //UNIT_TEST_GL(TestDrawRandomTextFiltering);
   //UNIT_TEST_GL(TestDrawSGIConvex);
   //UNIT_TEST_GL(TestDrawPoly);
   //UNIT_TEST_GL(TestDrawSolidRect);
   //UNIT_TEST_GL(TestDrawPathWithResourceCacheMiss);
   //UNIT_TEST_GL(TestDrawPathWithOffset);
   //UNIT_TEST_GL(TestDrawPathJoin);
   //UNIT_TEST_GL(TestDrawPathSolid1PX);
   //UNIT_TEST_GL(TestDrawPathSolid2PX);
   //UNIT_TEST_GL(TestDrawPathSolid);
   //UNIT_TEST_GL(TestDrawPathSymbol);
   //UNIT_TEST_GL(TestDrawOverlappedSymbolWithText);
   //UNIT_TEST_GL(TestDrawAnyRect);
   //UNIT_TEST_GL(TestDrawSector);
   //UNIT_TEST_GL(TestDrawPathSolidDiffWidth);
   //UNIT_TEST_GL(TestDrawPathZigZag);
   //UNIT_TEST_GL(TestDrawPathSolidWithZ);
   //UNIT_TEST_GL(TestDrawPathSolidWithClipRect);
   //UNIT_TEST_GL(TestDrawUtilsRect);
   //UNIT_TEST_GL(TestDrawUtilsRectFilledTexture);
   //UNIT_TEST_GL(TestDrawSymbolFiltering);
   //UNIT_TEST_GL(TestDrawCircle);
   //UNIT_TEST_GL(TestDrawImage);
}
