#include "../../base/SRC_FIRST.hpp"
#include "../../3party/sgitess/interface.h"

#include "../../yg/screen.hpp"
#include "../../yg/utils.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/skin.hpp"
#include "../../yg/pen_info.hpp"
#include "../../yg/circle_info.hpp"
#include "../../yg/text_element.hpp"
#include "../../yg/straight_text_element.hpp"
#include "../../yg/path_text_element.hpp"

#include "../../qt_tstfrm/macros.hpp"

#include "../../testing/testing.hpp"
#include <QtGui/QKeyEvent>

#include "../../base/math.hpp"
#include "../../base/string_utils.hpp"
#include "../../std/shared_ptr.hpp"

namespace
{
  struct TestDrawPoint
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawSymbol(m2::PointD(40, 40), 0, yg::EPosCenter, 0);
      p->drawSymbol(m2::PointD(40.5, 60), 0, yg::EPosCenter, 0);
      p->drawSymbol(m2::PointD(41, 80), 0, yg::EPosCenter, 0);
    }
  };

  struct TestDrawSolidRect
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->immDrawSolidRect(m2::RectF(0, 0, 100, 100), yg::Color(255, 0, 0, 255));
    }
  };

  struct TestDrawLine
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::PointD pts[3] =
      {
        m2::PointD(200, 200),
        m2::PointD(205, 200),
        m2::PointD(290, 200)
      };

//      double pat [] = {7, 7, 10, 10};
      double pat1 [] = {1, 1};
      p->drawPath(pts, 3, 0, p->skin()->mapPenInfo(yg::PenInfo(yg::Color(0xFF, 0xFF, 0xFF, 0xFF), 2, pat1, 2, 0)), 0);
    }
  };

  struct TestDrawPathBase
  {
    std::vector<std::vector<m2::PointD> > m_pathes;
    std::vector<double> m_pathOffsets;
    //std::vector<std::vector<double> > m_patterns;
    std::vector<yg::PenInfo> m_penInfos;
    std::vector<double> m_depthes;

    std::vector<double> m_axisPattern;
    yg::PenInfo m_axisPenInfo;
    bool m_drawAxis;

    void Init()
    {
      m_drawAxis = false;

      m_axisPattern.push_back(2);
      m_axisPattern.push_back(2);
      m_axisPenInfo = yg::PenInfo(yg::Color(0xFF, 0, 0, 0xFF), 2, &m_axisPattern[0], m_axisPattern.size(), 0);
    }

    void AddTest(std::vector<m2::PointD> const & points,
        std::vector<double> const & pattern,
        yg::Color const & color = yg::Color(255, 255, 255, 255),
        double width = 2,
        double depth = 0,
        double pathOffset = 0,
        double penOffset = 0)
    {
      m_pathes.push_back(points);
      m_pathOffsets.push_back(pathOffset);
      //m_patterns.push_back(pattern);
      m_penInfos.push_back(yg::PenInfo(color, width, pattern.empty() ? 0 : &pattern[0], pattern.size(), penOffset));
      m_depthes.push_back(depth);
    }

    std::vector<m2::PointD> & GetTestPoints(size_t i)
    {
      return m_pathes[i];
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      for (size_t i = 0; i < m_pathes.size(); ++i)
      {
        p->drawPath(&m_pathes[i][0], m_pathes[i].size(), m_pathOffsets[i], p->skin()->mapPenInfo(m_penInfos[i]), m_depthes[i]);
        if (m_drawAxis)
          p->drawPath(&m_pathes[i][0], m_pathes[i].size(), 0, p->skin()->mapPenInfo(m_axisPenInfo), m_depthes[i]);
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

  struct TestDrawPathWithSkinPageMiss : public TestDrawPathBase
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
          AddTest(points, pattern, yg::Color(128 + 128 / columns * i, 128 + 128 / rows * j, 0, 255), rand() % 15);
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

      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 150);

      pts.clear();
      pts.push_back(m2::PointD(200, 90));
      pts.push_back(m2::PointD(300, 90));

      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, -150);

      pts.clear();
      pts.push_back(m2::PointD(200, 100));
      pts.push_back(m2::PointD(400, 100));


      /// The path should start from -10px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, -10);

      pts.clear();
      pts.push_back(m2::PointD(200, 110));
      pts.push_back(m2::PointD(400, 110));

      /// The path should start from 0px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 0);

      pts.clear();
      pts.push_back(m2::PointD(200, 120));
      pts.push_back(m2::PointD(400, 120));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 60);

      pts.clear();
      pts.push_back(m2::PointD(200, 130));
      pts.push_back(m2::PointD(400, 130));

      /// The path should start from 0px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 0, -10);

      pts.clear();
      pts.push_back(m2::PointD(200, 140));
      pts.push_back(m2::PointD(400, 140));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 0, 0);

      pts.clear();
      pts.push_back(m2::PointD(200, 150));
      pts.push_back(m2::PointD(400, 150));

      /// The path should start from 60px path offset.
      AddTest(pts, pattern, yg::Color(0, 0, 0, 255), 4, 0, 0, 60);

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

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40);

      testPoints.clear();
      testPoints.push_back(m2::PointD(320, 300));
      testPoints.push_back(m2::PointD(380, 240));
      testPoints.push_back(m2::PointD(420, 369));
      testPoints.push_back(m2::PointD(520, 370));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40);

//      testPoints.clear();
//
//      testPoints.push_back(m2::PointD(460, 100));
//      testPoints.push_back(m2::PointD(505, 200));
//      testPoints.push_back(m2::PointD(600, 150));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 255, 255), 40);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 120));
//      testPoints.push_back(m2::PointD(160, 120));
//
//      testPattern.clear();
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 200));
//      testPoints.push_back(m2::PointD(50, 350));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 120));
//      testPoints.push_back(m2::PointD(250, 120));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 200));
//      testPoints.push_back(m2::PointD(400, 350));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(100, 200));
//      testPoints.push_back(m2::PointD(240, 220));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
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

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40, 0.5);

//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(150, 220));
      testPoints.push_back(m2::PointD(300, 220));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40, 0.5);

      testPattern.clear();
      testPoints.clear();

      testPoints.push_back(m2::PointD(200, 240));
      testPoints.push_back(m2::PointD(300, 240));

      AddTest(testPoints, testPattern, yg::Color(0, 0, 255, 255), 40, 0);
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

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40, 0.5);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->enableClipRect(true);
      p->setClipRect(m2::RectI(50, 70, 150, 150));
      base_t::DoDraw(p);
      p->endFrame();
      m2::RectI r(0, 0, 0, 0);
      p->beginFrame();
      p->setClipRect(r);
      p->enableClipRect(true);
      p->clear(yg::gl::Screen::s_bgColor);
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
        AddTest(testPoints, vector<double>(), yg::Color(255, 0, 0, 255), i + 1);
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
        AddTest(testPoints, pat, yg::Color(255, 0, 0, 255), i + 1);
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

      AddTest(testPoints, vector<double>(), yg::Color(255, 0, 0, 255), 6);

      for (unsigned i = 0; i < testPoints.size(); ++i)
        testPoints[i].y += 100;

      vector<double> pat;
      pat.push_back(10);
      pat.push_back(10);

      AddTest(testPoints, pat, yg::Color(255, 0, 0, 255), 6);
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

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 1);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 1);
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

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 2);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 2);
    }
  };


  struct TestDrawPathSolid : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

//      testPoints.push_back(m2::PointD(120, 200));
//      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40);
    }
  };

  struct TestDrawPoly
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
//      m2::PointD ptsStrip[5] = {m2::PointD(10, 10), m2::PointD(40, 10), m2::PointD(70, 10), m2::PointD(90, 60), m2::PointD(130, 30)};
//      p->drawTriangles(ptsStrip, 5, yg::TriangleStrip, p->skin()->mapColor(yg::Color(255, 0, 0, 255)));

//      m2::PointD ptsFan[5] = {m2::PointD(150, 20), m2::PointD(170, 80), m2::PointD(190, 100), m2::PointD(200, 80), m2::PointD(220, 60)};
//      p->drawTriangles(ptsFan, 5, yg::TriangleFan, p->skin()->mapColor(yg::Color(0, 255, 0, 255)));

      m2::PointD ptsList[6] = {m2::PointD(20, 80), m2::PointD(50, 120), m2::PointD(80, 80), m2::PointD(110, 80), m2::PointD(140, 120), m2::PointD(80, 120)};
      p->drawTrianglesList(ptsList, 6, /*yg::TriangleList, */p->skin()->mapColor(yg::Color(0, 0, 255, 255)), 0);
    }
  };

  /// Trying to draw polygon with more vertices that fits into internal buffer.
  struct TestDrawPolyOverflow
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
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
                           p->skin()->mapColor(yg::Color(0, 0, 255, 255)), 0);
    }
  };

  struct TestDrawText
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::PointD path[2] = {m2::PointD(100, 200), m2::PointD(1000, 200)};
      double pat[2] = {2, 2};
      p->drawPath(path, sizeof(path) / sizeof(m2::PointD), 0, p->skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, pat, 2, 0)), 0);

      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));

      p->drawText(fontDesc, m2::PointD(200, 200), yg::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(240, 200), yg::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(280, 200), yg::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(320, 200), yg::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(360, 200), yg::EPosAboveRight, "0", 0, true);
      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  struct TestDrawSingleSymbol
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, "X", 1, true);
    }
  };

  struct TestDrawEmptySymbol
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, " ", 1, true);
    }
  };

  struct TestDrawStringOnString
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      size_t const maxTimes = 10;
      size_t const yStep = 30;

      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));

      for (size_t i = 0; i < maxTimes; ++i)
        for (size_t j = 1; j <= i+1; ++j)
          p->drawText(fontDesc, m2::PointD(40, 10 + yStep * i), yg::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  struct TestDrawSingleSymbolAndSolidPath
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      vector<m2::PointD> path;
      path.push_back(m2::PointD(40, 50));
      path.push_back(m2::PointD(70, 50));

      double pat[] = { 2, 2 };
      yg::PenInfo penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
      yg::PenInfo solidPenInfo = yg::PenInfo(yg::Color(0xFF, 0, 0, 0xFF), 4, 0, 0, 0);

      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));

      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, "S", 0, true);
      p->drawPath(&path[0], path.size(), 0, p->skin()->mapPenInfo(solidPenInfo), 0);

    }
  };

  struct TestDrawMultiLineStringWithPosition
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(14, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));

      m2::PointD pt = m2::PointD(50, 150);

      p->drawText(fontDesc, pt, yg::EPosAboveRight, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(50, 300);

      p->drawText(fontDesc, pt, yg::EPosRight, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(50, 450);

      p->drawText(fontDesc, pt, yg::EPosUnderRight, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(400, 150);

      p->drawText(fontDesc, pt, yg::EPosAbove, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(400, 300);

      p->drawText(fontDesc, pt, yg::EPosCenter, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(400, 450);

      p->drawText(fontDesc, pt, yg::EPosUnder, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(750, 150);

      p->drawText(fontDesc, pt, yg::EPosAboveLeft, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(750, 300);

      p->drawText(fontDesc, pt, yg::EPosLeft, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);

      pt = m2::PointD(750, 450);

      p->drawText(fontDesc, pt, yg::EPosUnderLeft, "Simplicity is the ultimate sophistication", yg::maxDepth, true);
      p->drawRectangle(m2::Inflate(m2::RectD(pt, pt), m2::PointD(2, 2)), yg::Color(0, 0, 0, 255), yg::maxDepth);
    }
  };

  struct TestDrawString
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 150), yg::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };

  struct TestDrawStringWithColor
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(25, yg::Color(0, 0, 255, 255), true, yg::Color(255, 255, 255, 255));
      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, "Simplicity is the ultimate sophistication", 0, true);
    }
  };


  struct TestDrawUnicodeSymbols
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::FontDesc fontDesc(12);
      p->drawText(fontDesc, m2::PointD(40, 50), yg::EPosAboveRight, "Latin Symbol : A", 0, true);
      p->drawText(fontDesc, m2::PointD(40, 80), yg::EPosAboveRight, "Cyrillic Symbol : Ы", 0, true);
    }
  };

  struct TestDrawTextRect : TestDrawString
  {
    typedef TestDrawString base_t;
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::PointD startPt(40, 50);

      yg::StraightTextElement::Params params;
      params.m_depth = 0;
      params.m_fontDesc = yg::FontDesc(20, yg::Color(0, 0, 0, 0), true, yg::Color(255, 255, 255, 255));
      params.m_log2vis = false;
      params.m_pivot = startPt;
      params.m_position = yg::EPosAboveRight;
      params.m_glyphCache = p->glyphCache();
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication");
      yg::StraightTextElement ste(params);

      m2::RectD r = ste.roughBoundRect();
      p->drawRectangle(r, yg::Color(0, 0, 255, 255), 0);

      base_t::DoDraw(p);
    }
  };


  double calc_length(vector<m2::PointD> const & v)
  {
    double ret = 0.0;
    for (size_t i = 1; i < v.size(); ++i)
      ret += v[i-1].Length(v[i]);
    return ret;
  }

  struct TestDrawTextOnPathBigSymbols
  {
    vector<m2::PointD> m_path;
    string m_text;
    yg::PenInfo m_penInfo;

    void Init()
    {
      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(80, 200));
      m_path.push_back(m2::PointD(180, 250));

      m_text = "Syp";

      double pat[2] = {2, 2};
      m_penInfo = yg::PenInfo(yg::Color(0xFF, 0xFF, 0xFF, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), 0, p->skin()->mapPenInfo(m_penInfo), 1);
      yg::FontDesc fontDesc(30);

      p->drawPathText(fontDesc, &m_path[0], m_path.size(), m_text, calc_length(m_path), 0.0, yg::EPosLeft, 0);
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

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_testPoints[0], m_testPoints.size(), 0, p->skin()->mapPenInfo(yg::PenInfo(yg::Color(255, 255, 255, 255), 2, 0, 0, 0)), 0);
      yg::FontDesc fontDesc(20, yg::Color(0, 0, 0, 255), false);
      //m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";
      m_text = "Vinci";
      p->drawPathText(fontDesc, &m_testPoints[0], m_testPoints.size(), m_text.c_str(), calc_length(m_testPoints), m_pathOffset, yg::EPosLeft, 1);
    }
  };

  struct TestDrawTextOnPath
  {
    std::vector<m2::PointD> m_path;
    std::string m_text;
    yg::PenInfo m_penInfo;

    TestDrawTextOnPath()
    {

      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(100, 100));
      m_path.push_back(m2::PointD(600, 100));
      m_path.push_back(m2::PointD(400, 300));
      m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";

      double pat[] = { 2, 2 };
      m_penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);
      yg::FontDesc fontDesc(20);
      p->drawPathText(fontDesc, &m_path[0], m_path.size(), m_text, calc_length(m_path), 0.0, yg::EPosCenter, 0);
    }
  };

  struct TestDrawStraightTextElement
  {
    yg::PenInfo m_penInfo;
    vector<m2::PointD> m_path;
    TestDrawStraightTextElement()
    {
      m_path.push_back(m2::PointD(100, 200));
      m_path.push_back(m2::PointD(500, 200));
      double pat[] = { 2, 2 };
      m_penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::StraightTextElement::Params params;
      params.m_fontDesc = yg::FontDesc(20);
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication. Leonardo Da Vinci.");
      params.m_depth = 10;
      params.m_log2vis = false;
      params.m_glyphCache = p->glyphCache();
      params.m_pivot = m_path[0];
      params.m_position = yg::EPosRight;

      yg::StraightTextElement ste(params);

      p->drawPath(&m_path[0], m_path.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);
      ste.draw(p.get(), math::Identity<double, 3>());
    }
  };

  struct TestDrawPathTextElement
  {
    vector<m2::PointD> m_path;
    yg::PenInfo m_penInfo;

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
      m_penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      yg::PathTextElement::Params params;
      params.m_pts = &m_path[0];
      params.m_ptsCount = m_path.size();
      params.m_fullLength = calc_length(m_path);
      params.m_pathOffset = 0;
      params.m_fontDesc = yg::FontDesc(20);
      params.m_logText = strings::MakeUniString("Simplicity is the ultimate sophistication. Leonardo Da Vinci.");
      params.m_depth = 10;
      params.m_log2vis = false;
      params.m_glyphCache = p->glyphCache();
      params.m_pivot = m_path[0];
      params.m_position = yg::EPosCenter;

      yg::PathTextElement pte(params);

      p->drawPath(&m_path[0], m_path.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);
      pte.draw(p.get(), math::Identity<double, 3>());
    }
  };

  struct TestDrawTextOnPathZigZag
  {
    std::vector<m2::PointD> m_path;
    std::string m_text;
    yg::PenInfo m_penInfo;

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
      m_penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);
//      yg::FontDesc fontDesc(false, 10);
      yg::FontDesc fontDesc(20);
      p->drawPathText(fontDesc, &m_path[0], m_path.size(), m_text, calc_length(m_path), 0.0, yg::EPosCenter, 0);
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

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      TestDrawTextOnPath::DoDraw(p);

      p->drawPath(&m_pathAbove[0], m_pathAbove.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);
      p->drawPath(&m_pathUnder[0], m_pathUnder.size(), 0, p->skin()->mapPenInfo(m_penInfo), 0);

      double const len = calc_length(m_path);
      yg::FontDesc fontDesc(20);

      p->drawPathText(fontDesc, &m_pathAbove[0], m_pathAbove.size(), m_text, len, 0.0, yg::EPosAbove, 0);
      p->drawPathText(fontDesc, &m_pathUnder[0], m_pathUnder.size(), m_text, len, 0.0, yg::EPosUnder, 0);
    }
  };

  struct TestDrawTextOverflow
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        yg::FontDesc fontDesc(startSize + i);
        p->drawText(fontDesc, m2::PointD(10, startY), yg::EPosAboveRight, "Simplicity is the ultimate sophistication. Leonardo Da Vinci", 0,  true);
        startY += fontDesc.m_size;
      }
    }
  };

  struct TestDrawTextFiltering
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        yg::FontDesc fontDesc(startSize);
        p->drawText(fontDesc, m2::PointD(10, startY), yg::EPosAboveRight, "Simplicity is the ultimate sophistication. Leonardo Da Vinci", 100, true);
        p->drawText(fontDesc, m2::PointD(5, startY + (startSize + i) / 2), yg::EPosAboveRight, "This text should be filtered", 100, true);
        startY += startSize + i;
      }
    }
  };

  struct TestDrawRandomTextFiltering
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      char const * texts [] = {"Simplicity is the ultimate sophistication", "Leonardo Da Vinci"};

      int startSize = 20;
      int endSize = 40;

      int textsCount = 200;

      for (int i = 0; i < textsCount; ++i)
      {
        yg::FontDesc fontDesc(rand() % (endSize - startSize) + startSize,
                              yg::Color(rand() % 255, rand() % 255, rand() % 255, 255)
                              );
        p->drawText(
              fontDesc,
              m2::PointD(rand() % 500, rand() % 500),
              yg::EPosAboveRight,
              texts[rand() % (sizeof(texts) / sizeof(char*))],
              rand() % 10,
              true);
      }
    }
  };

  struct TestDrawUtilsRect
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      shared_ptr<yg::gl::RGBA8Texture> texture(new yg::gl::RGBA8Texture(512, 512));
      texture->randomize();

      p->immDrawRect(
          m2::RectF(0, 0, 512, 512),
          m2::RectF(0, 0, 1, 1),
          texture,
          true,
          yg::Color(255, 0, 0, 255),
          false);
    }
  };

  struct TestDrawUtilsRectFilledTexture
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      shared_ptr<yg::gl::RGBA8Texture> texture(new yg::gl::RGBA8Texture(512, 512));
      texture->fill(yg::Color(0, 255, 0, 255));

      p->immDrawRect(
          m2::RectF(0, 0, 512, 512),
          m2::RectF(0, 0, 1, 1),
          texture,
          true,
          yg::Color(255, 0, 0, 255),
          false);
    }
  };

  struct TestDrawOverlappedSymbolWithText
  {
  public:
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->setOverlay(make_shared_ptr(new yg::Overlay()));
      p->overlay()->setCouldOverlap(false);

      p->drawSymbol(m2::PointD(200, 200), "current-position", yg::EPosUnder, yg::maxDepth);
      p->drawText(yg::FontDesc(), m2::PointD(200, 200), yg::EPosAbove, "Test Text", yg::maxDepth, true);

      p->drawSymbol(m2::PointD(180, 200), "current-position", yg::EPosUnder, yg::maxDepth);
      p->drawText(yg::FontDesc(), m2::PointD(180, 200), yg::EPosAbove, "Test Text", yg::maxDepth, true);

      p->endFrame();
      p->beginFrame();

      p->overlay()->draw(p.get(), math::Identity<double, 3>());
      p->resetOverlay();
    }
  };

  struct TestDrawAnyRect
  {
  public:
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::AnyRectD r[3] =
      {
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, 0, 50, 20)),
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, -10, 50, 10)),
        m2::AnyRectD(m2::PointD(100, 100), math::pi / 6, m2::RectD(0, -22, 50, -2))
      };

      p->drawRectangle(r[0], yg::Color(255, 0, 0, 128), yg::maxDepth - 2);
      if (!r[0].IsIntersect(r[1]))
        p->drawRectangle(r[1], yg::Color(0, 255, 0, 128), yg::maxDepth - 1);
      if (!r[0].IsIntersect(r[2]))
        p->drawRectangle(r[1], yg::Color(0, 0, 255, 128), yg::maxDepth);
    }
  };

  struct TestDrawSector
  {
  public:
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawArc(m2::PointD(100, 100), 0, math::pi * 2, 30, yg::Color(0, 0, 255, 128), 12000);
      p->fillSector(m2::PointD(100, 100), 0, math::pi * 2, 30, yg::Color(0, 0, 255, 64), 12000);
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

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      double inputDataPat[] = {10, 0};
      yg::PenInfo inputDataRule(yg::Color::fromARGB(0xFF000000), 6, inputDataPat, 2, 0);

      double triangleFanPat[] = {10, 10};
      yg::PenInfo triangleFanRule(yg::Color::fromARGB(0xFFFF0000), 5, triangleFanPat, 2, 0);

      double triangleStripPat[] = {10, 10};
      yg::PenInfo triangleStripRule(yg::Color::fromARGB(0xFF00FF00), 4, triangleStripPat, 2, 0);

      double triangleListPat[] = {10, 10};
      yg::PenInfo triangleListRule(yg::Color::fromARGB(0xFF0000FF), 3, triangleListPat, 2, 0);

      double lineLoopPat[] = {2, 2};
      yg::PenInfo lineLoopRule(yg::Color::fromARGB(0xFF00FFFF), 2, lineLoopPat, 2, 0);

      uint32_t inputDataID = p->skin()->mapPenInfo(inputDataRule);
      uint32_t triangleFanID = p->skin()->mapPenInfo(triangleFanRule);
      /*uint32_t triangleStripID = */p->skin()->mapPenInfo(triangleStripRule);
      uint32_t triangleListID = p->skin()->mapPenInfo(triangleListRule);
      uint32_t lineLoopID = p->skin()->mapPenInfo(lineLoopRule);

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
    void DoDraw(shared_ptr<yg::gl::Screen> const & p)
    {
      for (int i = 0; i < 40; ++i)
        p->drawSymbol(m2::PointD(100 + i, 100), "hospital", yg::EPosCenter, 0);
    }
  };

  struct TestDrawCircle
  {
    void DoDraw(shared_ptr<yg::gl::Screen> const & p)
    {
      p->drawCircle(m2::PointD(200, 200), p->skin()->mapCircleInfo(yg::CircleInfo(10, yg::Color(255, 0, 0, 255))), yg::EPosCenter, 100);
      p->drawCircle(m2::PointD(100, 200), p->skin()->mapCircleInfo(yg::CircleInfo(10, yg::Color(255, 0, 0, 255), true, 4, yg::Color(255, 255, 255, 255))), yg::EPosCenter, 100);
    }
  };

   UNIT_TEST_GL(TestDrawPolyOverflow);
   UNIT_TEST_GL(TestDrawText);
   UNIT_TEST_GL(TestDrawSingleSymbol);
   UNIT_TEST_GL(TestDrawEmptySymbol);
   UNIT_TEST_GL(TestDrawSingleSymbolAndSolidPath);
   UNIT_TEST_GL(TestDrawMultiLineStringWithPosition);
   UNIT_TEST_GL(TestDrawString);
   UNIT_TEST_GL(TestDrawStringWithColor);
   UNIT_TEST_GL(TestDrawUnicodeSymbols);
   UNIT_TEST_GL(TestDrawTextRect);
   UNIT_TEST_GL(TestDrawStringOnString);
   UNIT_TEST_GL(TestDrawTextOnPathInteractive);
   UNIT_TEST_GL(TestDrawTextOnPathBigSymbols);
   UNIT_TEST_GL(TestDrawTextOnPath);
   UNIT_TEST_GL(TestDrawTextOnPathZigZag);
   UNIT_TEST_GL(TestDrawTextOnPathWithOffset);
   UNIT_TEST_GL(TestDrawStraightTextElement);
   UNIT_TEST_GL(TestDrawPathTextElement);
   UNIT_TEST_GL(TestDrawTextOverflow);
   UNIT_TEST_GL(TestDrawTextFiltering);
   UNIT_TEST_GL(TestDrawRandomTextFiltering);
//   UNIT_TEST_GL(TestDrawSGIConvex);
   UNIT_TEST_GL(TestDrawPoly);
   UNIT_TEST_GL(TestDrawSolidRect);
   UNIT_TEST_GL(TestDrawPathWithSkinPageMiss);
   UNIT_TEST_GL(TestDrawPathWithOffset);
   UNIT_TEST_GL(TestDrawPathJoin);
   UNIT_TEST_GL(TestDrawPathSolid1PX);
   UNIT_TEST_GL(TestDrawPathSolid2PX);
   UNIT_TEST_GL(TestDrawPathSolid);
   UNIT_TEST_GL(TestDrawOverlappedSymbolWithText);
   UNIT_TEST_GL(TestDrawAnyRect);
   UNIT_TEST_GL(TestDrawSector);
   UNIT_TEST_GL(TestDrawPathSolidDiffWidth);
   UNIT_TEST_GL(TestDrawPathZigZag);
   UNIT_TEST_GL(TestDrawPathSolidWithZ);
   UNIT_TEST_GL(TestDrawPathSolidWithClipRect);
   UNIT_TEST_GL(TestDrawUtilsRect);
   UNIT_TEST_GL(TestDrawUtilsRectFilledTexture);
   UNIT_TEST_GL(TestDrawSymbolFiltering);
   UNIT_TEST_GL(TestDrawCircle);
}
