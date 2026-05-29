#include "testing/testing.hpp"

#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/visual_params.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include <QtGui/QPainter>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
bool IsSmooth(m2::SplineEx const & spline)
{
  for (size_t i = 0, sz = spline.GetDirections().size(); i + 1 < sz; ++i)
    if (!df::IsValidSplineTurn(spline.GetDirections()[i], spline.GetDirections()[i + 1]))
      return false;
  return true;
}

m2::SplineEx BuildRounded(std::vector<m2::PointD> const & pts)
{
  m2::SplineEx spline;
  for (auto const & p : pts)
    df::AddPointAndRound(spline, p);
  return spline;
}
}  // namespace

UNIT_TEST(Rounding_Spline)
{
  df::VisualParams::Init(1.0, 1024);  // AddPointAndRound reads GetVisualScale().

  // For rounded corners we assert smoothness and that rounding added points, but not the exact count:
  // the number of arc steps is ceil(turn / kValidPathSplineTurn), which is FP/platform-dependent when
  // the turn is an exact multiple of the step (e.g. 90 or 45 deg), so the total can differ by +-1.
  m2::SplineEx spline1;
  df::AddPointAndRound(spline1, m2::PointD(0, 200));
  df::AddPointAndRound(spline1, m2::PointD(0, 0));
  df::AddPointAndRound(spline1, m2::PointD(200, 0));
  TEST(IsSmooth(spline1), ());
  TEST_GREATER(spline1.GetSize(), 7, ());  // The 90 deg corner is rounded.

  m2::SplineEx spline2;
  df::AddPointAndRound(spline2, m2::PointD(-200, 0));
  df::AddPointAndRound(spline2, m2::PointD(0, 0));
  df::AddPointAndRound(spline2, m2::PointD(200, 200));
  df::AddPointAndRound(spline2, m2::PointD(400, 200));
  TEST(IsSmooth(spline2), ());
  TEST_GREATER(spline2.GetSize(), 7, ());  // Both 45 deg corners are rounded.

  m2::SplineEx spline3;
  df::AddPointAndRound(spline3, m2::PointD(200, 100));
  df::AddPointAndRound(spline3, m2::PointD(0, 0));
  df::AddPointAndRound(spline3, m2::PointD(200, 0));
  TEST(!IsSmooth(spline3), ());
  TEST_EQUAL(spline3.GetSize(), 3, ());

  m2::SplineEx spline4;
  df::AddPointAndRound(spline4, m2::PointD(-200, 5));
  df::AddPointAndRound(spline4, m2::PointD(0, 0));
  df::AddPointAndRound(spline4, m2::PointD(200, 5));
  TEST(IsSmooth(spline4), ());
  TEST_EQUAL(spline4.GetSize(), 3, ());

  m2::SplineEx spline5;
  df::AddPointAndRound(spline5, m2::PointD(200, 5));
  df::AddPointAndRound(spline5, m2::PointD(0, 0));
  df::AddPointAndRound(spline5, m2::PointD(200, -5));
  TEST(!IsSmooth(spline5), ());
  TEST_EQUAL(spline5.GetSize(), 3, ());
}

namespace
{
struct RoundingCase
{
  std::string m_name;
  std::vector<m2::PointD> m_points;  // Raw (sharp) input vertices fed to AddPointAndRound.
};

// Custom sharp-edge geometries spanning the algorithm's behaviour: gentle turns left untouched,
// right angles rounded, very acute corners where it gives up, and multi-corner chains.
std::vector<RoundingCase> GetVisualCases()
{
  return {
      {"90 deg corner", {{0, 200}, {0, 0}, {200, 0}}},
      {"45 deg bends", {{-200, 0}, {0, 0}, {200, 200}, {400, 200}}},
      {"Acute hairpin", {{200, 100}, {0, 0}, {200, 0}}},
      {"Gentle <15 deg", {{-200, 5}, {0, 0}, {200, 5}}},
      {"Sharp V", {{-150, 150}, {0, 0}, {150, 150}}},
      {"Zigzag", {{0, 0}, {120, 90}, {240, 0}, {360, 90}, {480, 0}}},
      {"U-turn", {{200, 0}, {0, 0}, {0, 120}, {200, 120}}},
      {"S-curve", {{0, 0}, {150, 0}, {220, 110}, {370, 110}}},
  };
}

void DrawCase(QPainter & painter, QRectF const & cell, RoundingCase const & c)
{
  auto const rounded = BuildRounded(c.m_points);
  auto const & path = rounded.GetPath();

  // Combined bbox over input and rounded points so both fit in the cell.
  m2::RectD bbox;
  for (auto const & p : c.m_points)
    bbox.Add(p);
  for (auto const & p : path)
    bbox.Add(p);

  double const bw = std::max(1.0, bbox.SizeX());
  double const bh = std::max(1.0, bbox.SizeY());
  double const header = 22.0, margin = 16.0;
  double const scale = std::min((cell.width() - 2 * margin) / bw, (cell.height() - header - 2 * margin) / bh);
  double const ox = cell.left() + margin;
  double const oy = cell.top() + header + margin;
  auto const toScreen = [&](m2::PointD const & p)
  {
    return QPointF(ox + (p.x - bbox.minX()) * scale, oy + (bbox.maxY() - p.y) * scale);
  };  // flip Y (screen grows down)

  painter.setPen(QPen(QColor(210, 210, 210)));
  painter.setBrush(Qt::NoBrush);
  painter.drawRect(cell.adjusted(1, 1, -1, -1));

  bool const smooth = IsSmooth(rounded);
  std::string const info = c.m_name + "  in " + std::to_string(c.m_points.size()) + " -> out " +
                           std::to_string(rounded.GetSize()) + (smooth ? "  smooth" : "  SHARP");
  painter.setPen(Qt::black);
  painter.drawText(QPointF(cell.left() + margin, cell.top() + 16), QString::fromStdString(info));

  // Input polyline: gray dashed, vertices as hollow red circles.
  QPen inPen(QColor(160, 160, 160));
  inPen.setStyle(Qt::DashLine);
  painter.setPen(inPen);
  for (size_t i = 1; i < c.m_points.size(); ++i)
    painter.drawLine(toScreen(c.m_points[i - 1]), toScreen(c.m_points[i]));
  painter.setPen(QPen(QColor(220, 60, 60)));
  for (auto const & p : c.m_points)
    painter.drawEllipse(toScreen(p), 4, 4);

  // Rounded output: blue solid, points as filled green dots.
  QPen outPen(QColor(40, 90, 220));
  outPen.setWidthF(2.0);
  painter.setPen(outPen);
  for (size_t i = 1; i < path.size(); ++i)
    painter.drawLine(toScreen(path[i - 1]), toScreen(path[i]));
  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(30, 160, 60));
  for (auto const & p : path)
    painter.drawEllipse(toScreen(p), 2.5, 2.5);
}

void RenderRoundingCases(QPaintDevice * device)
{
  QPainter painter(device);
  int const w = device->width();
  int const h = device->height();
  painter.fillRect(QRectF(0, 0, w, h), Qt::white);
  painter.setRenderHint(QPainter::Antialiasing, true);

  auto const cases = GetVisualCases();
  int const n = static_cast<int>(cases.size());
  int const cols = std::min(4, n);
  int const rows = (n + cols - 1) / cols;
  double const cellW = static_cast<double>(w) / cols;
  double const cellH = static_cast<double>(h - 18) / rows;  // bottom strip for the legend
  for (int i = 0; i < n; ++i)
    DrawCase(painter, QRectF((i % cols) * cellW, (i / cols) * cellH, cellW, cellH), cases[i]);

  painter.setPen(Qt::black);
  painter.drawText(QPointF(8, h - 8), QString::fromLatin1("gray dashed = sharp input, red = input vertices, "
                                                          "blue = AddPointAndRound output, green = output vertices"));
}
}  // namespace

// Visual inspection of AddPointAndRound corner rounding (see drape_tests/glyph_mng_tests for the pattern).
// Draw test cases from Rounding_Spline.
UNIT_TEST(Rounding_Spline_Visual)
{
  df::VisualParams::Init(1.0, 1024);  // AddPointAndRound reads GetVisualScale().

  RunTestLoop("PathText AddPointAndRound corner rounding", &RenderRoundingCases, true /* autoExit; false to inspect */);
}
