#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/oglcontext.hpp"
#include "drape/support_manager.hpp"

#include "platform/platform.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include <QtGui/QPainter>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace path_text_test
{
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

using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, Rounding_Spline)
{
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
class CurrentOGLContext : public dp::OGLContext
{
public:
  void MakeCurrent() override {}
  void DoneCurrent() override {}
  void Present() override {}
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ForgetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}
  void ApplyFramebuffer(std::string const &) override {}
};
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, PathText_Repositioning)
{
  RunTestInOpenGLOffscreenEnvironment("Contour caption placement", []()
  {
    CurrentOGLContext graphicsContext;
    graphicsContext.Init(dp::ApiVersion::OpenGLES3);
    dp::SupportManager::Instance().Init(make_ref(&graphicsContext));

    dp::TextureManager textures;
    dp::TextureManager::Params params;
    params.m_resPostfix = df::VisualParams::GetResourcePostfix(1.0);
    params.m_colors = "colors.txt";
    params.m_patterns = "patterns.txt";
    params.m_glyphMngParams.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
    params.m_glyphMngParams.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
    params.m_glyphMngParams.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
    GetPlatform().GetFontNames(params.m_glyphMngParams.m_fonts);
    textures.Init(make_ref(&graphicsContext), params);

    auto const makeLayout = [&]()
    {
      return make_unique_dp<df::PathTextLayout>(m2::PointD::Zero(), "100", 12.0f, make_ref(&textures),
                                                StringUtf8Multilang::kUnsupportedLanguageCode);
    };
    double const width = makeLayout()->GetPixelLength();
    TEST_GREATER(width, 0.0, ());
    ScreenBase screen;
    screen.OnSize(0, 0, 1024, 1024);
    screen.SetFromRect(m2::AnyRectD(m2::RectD(-512, -512, 512, 512)));
    auto const makeContext = [&](m2::SharedSpline const & spline, bool allowRepositioning, double xOffset = 0.0)
    {
      auto context = std::make_unique<df::PathTextContext>(spline, xOffset, allowRepositioning);
      context->SetLayout(makeLayout(), 1.0);
      context->Update(screen);
      return context;
    };
    gpu::TTextDynamicVertexBuffer buffer;
    m2::PointD pivot;

    // A sharp midpoint must not prevent a caption on either adjacent straight stretch.
    double const arm = 4.0 * width;
    m2::SharedSpline const bent({{-arm, 0}, {0, 0}, {-0.5 * arm, std::sqrt(0.75) * arm}});
    auto fixed = makeContext(bent, false);
    auto movable = makeContext(bent, true);
    TEST_EQUAL(fixed->GetOffsets().size(), 1, ());
    TEST_EQUAL(movable->GetOffsets(), fixed->GetOffsets(), ());
    TEST(!fixed->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    TEST(movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    TEST_GREATER(pivot.SquaredLength(), 1e-8, ());
    TEST_EQUAL(buffer.size(), 4 * movable->GetLayout()->GetGlyphCount(), ());
    m2::PointD const recoveredPivot = pivot;
    movable->BeforeUpdate();
    movable->Update(screen);
    TEST(movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    TEST(pivot.EqualDxDy(recoveredPivot, 1e-8), (pivot, recoveredPivot));

    // Successful original placements retain their centers and repetition count.
    m2::SharedSpline const straight({{-600, 0}, {600, 0}});
    fixed = makeContext(straight, false);
    movable = makeContext(straight, true);
    TEST_GREATER(movable->GetOffsets().size(), 1, ());
    TEST_EQUAL(movable->GetOffsets(), fixed->GetOffsets(), ());
    for (size_t i = 0; i < movable->GetOffsets().size(); ++i)
    {
      m2::PointD originalPivot;
      TEST(fixed->CacheDynamicGeometry(i, screen, 0.0f, originalPivot, buffer), (i));
      TEST(movable->CacheDynamicGeometry(i, screen, 0.0f, pivot, buffer), (i));
      TEST(pivot.EqualDxDy(originalPivot, 1e-8), (i, pivot, originalPivot));
    }

    // Every fitting position is too curved; retries must keep rejecting the caption.
    std::vector<m2::PointD> circle;
    for (size_t i = 0; i <= 64; ++i)
    {
      double const angle = 2.0 * math::pi * i / 64;
      circle.emplace_back(std::cos(angle) * width / math::pi, std::sin(angle) * width / math::pi);
    }
    movable = makeContext(m2::SharedSpline(std::move(circle)), true);
    TEST_EQUAL(movable->GetOffsets().size(), 1, ());
    TEST(!movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());

    // CalculatePositions admits paths that become long enough on the next zoom; they still
    // cannot render at this zoom even with alternative placements.
    movable = makeContext(m2::SharedSpline({{0, 0}, {0.75 * width, 0}}), true);
    TEST_EQUAL(movable->GetOffsets().size(), 1, ());
    TEST(!movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());

    // Repositioning a wrapped world copy preserves the same relative global pivot.
    auto wrapped = makeContext(bent, true, 360.0);
    TEST(wrapped->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    TEST(pivot.EqualDxDy(recoveredPivot + m2::PointD(360.0, 0.0), 1e-8), (pivot, recoveredPivot));
    auto const checkAnchor = [&]()
    {
      for (auto const & vertex : buffer)
      {
        TEST_EQUAL(vertex.m_position.x, static_cast<float>(pivot.x * df::kShapeCoordScalar), ());
        TEST_EQUAL(vertex.m_position.y, static_cast<float>(pivot.y * df::kShapeCoordScalar), ());
      }
    };
    checkAnchor();

    screen.ApplyPerspective(0.3, 0.6, 0.7);
    movable = makeContext(bent, true);
    TEST(movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    TEST(!screen.IsReverseProjection3d(screen.GtoP(pivot)), (pivot));
    checkAnchor();

    wrapped.reset();
    movable.reset();
    fixed.reset();
    textures.Release();
  });
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
UNIT_CLASS_TEST(VisualParamsFixture, Rounding_Spline_Visual)
{
  RunTestLoop("PathText AddPointAndRound corner rounding", &RenderRoundingCases, true /* autoExit; false to inspect */);
}

}  // namespace path_text_test
