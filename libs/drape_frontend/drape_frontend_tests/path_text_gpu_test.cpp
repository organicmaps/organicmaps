#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/path_text_handle.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/constants.hpp"
#include "drape/support_manager.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/math.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include <cmath>
#include <memory>
#include <vector>

namespace path_text_gpu_test
{
using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, PathText_Repositioning)
{
  RunTestInOpenGLOffscreenEnvironment("Contour caption placement", []()
  {
    df::test_support::CurrentOGLContext graphicsContext;
    graphicsContext.Init(dp::ApiVersion::OpenGLES3);
    dp::SupportManager::Instance().Init(make_ref(&graphicsContext));

    dp::TextureManager textures;
    df::test_support::InitTextureManager(make_ref(&graphicsContext), textures);

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

    // A moved caption stays on a closer screen, though a position nearer the corner fits there.
    double const shortArm = 1.6 * width;
    m2::SharedSpline const shortBent({{-shortArm, 0}, {0, 0}, {-0.5 * shortArm, std::sqrt(0.75) * shortArm}});
    ScreenBase closer = screen;
    closer.SetFromRect(m2::AnyRectD(m2::RectD(-256, -256, 256, 256)));
    movable = makeContext(shortBent, true);
    TEST(movable->CacheDynamicGeometry(0, screen, 0.0f, pivot, buffer), ());
    m2::PointD const movedPivot = pivot;
    movable->BeforeUpdate();
    movable->Update(closer);
    TEST(movable->CacheDynamicGeometry(0, closer, 0.0f, pivot, buffer), ());
    TEST(pivot.EqualDxDy(movedPivot, 1e-8), (pivot, movedPivot));
    auto fresh = std::make_unique<df::PathTextContext>(shortBent, 0.0, true);
    fresh->SetLayout(makeLayout(), 1.0);
    fresh->Update(closer);
    TEST(fresh->CacheDynamicGeometry(0, closer, 0.0f, pivot, buffer), ());
    TEST(!pivot.EqualDxDy(movedPivot, 1e-8), (pivot, movedPivot));

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
  });
}

// The handle culls a moved caption where it is drawn: its original pivot can leave the screen while it stays visible.
UNIT_CLASS_TEST(VisualParamsFixture, PathText_MovedCaptionCulling)
{
  RunTestInOpenGLOffscreenEnvironment("Moved contour caption culling", []()
  {
    df::test_support::CurrentOGLContext graphicsContext;
    graphicsContext.Init(dp::ApiVersion::OpenGLES3);
    dp::SupportManager::Instance().Init(make_ref(&graphicsContext));

    dp::TextureManager textures;
    df::test_support::InitTextureManager(make_ref(&graphicsContext), textures);

    auto layout = make_unique_dp<df::PathTextLayout>(m2::PointD::Zero(), "100", 12.0f, make_ref(&textures),
                                                     StringUtf8Multilang::kUnsupportedLanguageCode);
    double const width = layout->GetPixelLength();
    // The sharp midpoint at the origin moves the caption onto a straight arm.
    double const arm = 4.0 * width;
    auto context = std::make_shared<df::PathTextContext>(
        m2::SharedSpline({{-arm, 0}, {0, 0}, {-0.5 * arm, std::sqrt(0.75) * arm}}), 0.0, true /* allowRepositioning */);
    context->SetLayout(std::move(layout), 1.0);
    df::PathTextHandle handle(dp::OverlayID(), context, 0.0f /* depth */, 0 /* textIndex */, 0 /* priority */,
                              make_ref(&textures), 0 /* minVisibleScale */, false /* isBillboard */);
    textures.UpdateDynamicTextures(make_ref(&graphicsContext));

    ScreenBase screen;
    screen.OnSize(0, 0, 1024, 1024);
    screen.SetFromRect(m2::AnyRectD(m2::RectD(-512, -512, 512, 512)));
    handle.BeforeUpdate();
    TEST(handle.Update(screen), ());
    m2::PointD const moved = screen.PtoG(handle.GetPixelRect(screen, false).Center());
    TEST_GREATER(moved.Length(), 0.5 * width, (moved));

    ScreenBase zoomed;
    zoomed.OnSize(0, 0, 1024, 1024);
    zoomed.SetFromRect(m2::AnyRectD(m2::RectD(moved.x - 5, moved.y - 5, moved.x + 5, moved.y + 5)));
    m2::RectD extended = zoomed.PixelRect();
    extended.Inflate(dp::kScreenPixelRectExtension, dp::kScreenPixelRectExtension);
    TEST(!extended.IsPointInside(zoomed.GtoP(m2::PointD::Zero())), ("The original pivot must leave the screen"));
    handle.BeforeUpdate();
    TEST(handle.Update(zoomed), ());
    auto const rect = handle.GetPixelRect(zoomed, false);
    TEST(zoomed.PixelRect().IsPointInside(rect.Center()), (rect));
  });
}
}  // namespace path_text_gpu_test
