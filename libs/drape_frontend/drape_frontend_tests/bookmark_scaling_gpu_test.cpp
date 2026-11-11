#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/user_mark_shapes.hpp"

#include "base/scope_guard.hpp"

#include <QtGui/QColor>

#include <utility>

namespace bookmark_scaling_gpu_test
{
using df::test_support::VisualParamsFixture;

namespace
{
class UserMarksShape : public df::MapShape
{
public:
  UserMarksShape(df::TileKey const & tile, df::MarksSource const & source, df::UserMarksRenderCollection const & marks)
    : m_tile(tile)
    , m_source(source)
    , m_marks(marks)
  {}

  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override
  {
    df::CacheUserMarks(context, m_tile, textures, m_source, m_marks, *batcher.get());
  }

private:
  df::TileKey const & m_tile;
  df::MarksSource const & m_source;
  df::UserMarksRenderCollection const & m_marks;
};

struct IconBounds
{
  QRect m_bookmark;
  QRect m_control;
};

IconBounds GetIconBounds(QImage const & image)
{
  IconBounds bounds;
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x)
    {
      auto const color = image.pixelColor(x, y);
      if (color.red() > color.blue() + 50 && color.red() > color.green() + 50)
        bounds.m_bookmark |= QRect(x, y, 1, 1);
      else if (color.blue() > color.red() + 50 && color.blue() > color.green() + 50)
        bounds.m_control |= QRect(x, y, 1, 1);
    }
  return bounds;
}
}  // namespace

UNIT_CLASS_TEST(VisualParamsFixture, BookmarkIconsFollowFontScaleGpuTest)
{
  auto & visualParams = df::VisualParams::Instance();
  double const oldScale = visualParams.GetFontScale();
  SCOPE_GUARD(restoreScale, [&] { visualParams.SetFontScale(oldScale); });

  int constexpr kZoom = 14;
  auto const tile = df::GetTileKeyByPoint({0, 0}, kZoom);
  auto const center = tile.GetWrappedDataRect().Center();
  double const pixelToGlobalScale = df::GetScreenScale(kZoom);
  kml::MarkGroupId constexpr kGroupId = 1;
  df::UserGroupsVisibilitySet const visibility = {kGroupId};
  df::MarksIDGroups groups;
  auto ids = make_unique_dp<df::IDCollections>();
  df::UserMarksRenderCollection marks;
  auto const addMark = [&](kml::MarkId id, double x, dp::Color color, bool usesFontScale)
  {
    ids->m_markIds.push_back(id);
    auto mark = make_unique_dp<df::UserMarkRenderParams>();
    mark->m_markId = id;
    mark->m_pivot = center + m2::PointD(x, 0) * pixelToGlobalScale;
    mark->m_anchor = dp::Bottom;
    mark->m_depthTestEnabled = false;
    mark->m_isSymbolSelectable = false;
    mark->m_symbolUsesFontScale = usesFontScale;
    mark->m_customColor = color;
    mark->m_symbolNames = make_unique_dp<df::UserPointMark::SymbolNameZoomInfo>();
    mark->m_symbolNames->emplace(1, "bookmark-default-m");
    marks.emplace(id, std::move(mark));
  };
  addMark(1, -64, dp::Color::Red(), true /* usesFontScale */);
  addMark(2, 64, dp::Color::Blue(), false /* usesFontScale */);
  groups.emplace(kGroupId, std::move(ids));
  df::MarksSource source(&visibility);
  source.AddGroup(make_ref(&groups));

  uint32_t constexpr kHeight = 192;
  // Reuse the same cached mark parameters, as a Dynamic Type change does.
  auto const render = [&](double requestedScale)
  {
    visualParams.SetFontScale(requestedScale);
    df::test_support::ShapeTestFixture fixture;
    fixture.SetWorldToPixelScale(static_cast<float>(1.0 / pixelToGlobalScale));
    fixture.Render("Bookmark font scaling", 256, kHeight,
                   [&](auto & f) { f.AddShape(make_unique_dp<UserMarksShape>(tile, source, marks)); });

    auto const bounds = GetIconBounds(fixture.GetLastImage());
    TEST(!bounds.m_bookmark.isEmpty(), ("Bookmark sprite was not rendered", requestedScale));
    TEST(!bounds.m_control.isEmpty(), ("Control sprite was not rendered", requestedScale));
    return bounds;
  };

  auto const baseline = render(1.0);
  for (double const requestedScale : {1.4, 1.8, 2.88, 0.8, 1.0})
  {
    auto const bounds = render(requestedScale);
    double const scale = visualParams.GetFontScale();
    TEST(bounds.m_control == baseline.m_control, ("Control sprite changed", requestedScale));
    TEST_ALMOST_EQUAL_ABS(static_cast<double>(bounds.m_bookmark.width()), baseline.m_bookmark.width() * scale, 2.0,
                          (requestedScale));
    TEST_ALMOST_EQUAL_ABS(static_cast<double>(bounds.m_bookmark.height()), baseline.m_bookmark.height() * scale, 2.0,
                          (requestedScale));
    // The projection puts the tip above the icon. Atlas padding from the anchor to the first pixel scales too.
    double constexpr kAnchorY = kHeight / 2.0;
    double const expectedTop = kAnchorY + (baseline.m_bookmark.top() - kAnchorY) * scale;
    TEST_ALMOST_EQUAL_ABS(static_cast<double>(bounds.m_bookmark.top()), expectedTop, 2.0,
                          ("Pin tip moved", requestedScale));
  }
}
}  // namespace bookmark_scaling_gpu_test
