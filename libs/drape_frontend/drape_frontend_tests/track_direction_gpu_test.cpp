#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/color.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include <QtGui/QColor>
#include <QtGui/QImage>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>

namespace track_direction_gpu_test
{
uint32_t CountPixels(QImage const & image, int centerY, std::function<bool(QColor const &)> const & predicate)
{
  uint32_t count = 0;
  for (int y = centerY - 4; y <= centerY + 4; ++y)
  {
    for (int x = 70; x < image.width() - 70; ++x)
      if (predicate(image.pixelColor(x, y)))
        ++count;
  }
  return count;
}

int GetPixelSpanY(QImage const & image, int centerY, std::function<bool(QColor const &)> const & predicate)
{
  int minY = image.height();
  int maxY = -1;
  for (int y = centerY - 5; y <= centerY + 5; ++y)
  {
    for (int x = 70; x < image.width() - 70; ++x)
    {
      if (!predicate(image.pixelColor(x, y)))
        continue;
      minY = std::min(minY, y);
      maxY = std::max(maxY, y);
    }
  }
  return maxY < minY ? 0 : maxY - minY + 1;
}

uint32_t CountEndpointPixels(QImage const & image, int centerX, int centerY,
                             std::function<bool(QColor const &)> const & predicate)
{
  uint32_t count = 0;
  for (int y = centerY - 6; y <= centerY + 6; ++y)
  {
    for (int x = centerX - 6; x <= centerX + 6; ++x)
      if (predicate(image.pixelColor(x, y)))
        ++count;
  }
  return count;
}

double DistanceToPolyline(m2::PointD const & point, std::vector<m2::PointD> const & polyline)
{
  double squareDistance = std::numeric_limits<double>::max();
  for (size_t i = 1; i < polyline.size(); ++i)
  {
    m2::ParametrizedSegment<m2::PointD> const segment(polyline[i - 1], polyline[i]);
    squareDistance = std::min(squareDistance, segment.ClosestPointTo(point).SquaredLength(point));
  }
  return std::sqrt(squareDistance);
}
}  // namespace track_direction_gpu_test

UNIT_TEST(TrackDirectionArrowContrastGpuTest)
{
  using namespace track_direction_gpu_test;

  uint32_t constexpr kWidth = 400;
  uint32_t constexpr kHeight = 240;
  int constexpr kZoom = 11;
  df::VisualParams::Init(1.0 /* visualScale */, 512 /* tileSize */);
  double const pixelToGlobalScale = df::GetScreenScale(kZoom);
  auto const tileKey = df::GetTileKeyByPoint(mercator::Bounds::FullRect().Center(), kZoom);

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1.0 / pixelToGlobalScale));
  fixture.Render("Track direction arrow contrast", kWidth, kHeight,
                 [tileKey, pixelToGlobalScale](df::test_support::ShapeTestFixture & f)
  {
    struct Row
    {
      double m_yInPixels;
      dp::Color m_color;
    };
    Row const rows[] = {{90.0, dp::Color::Black()},
                        {30.0, dp::Color::White()},
                        {-30.0, dp::Color::Red()},
                        {-90.0, dp::Color::Yellow()}};

    kml::MarkGroupId constexpr kGroupId = 1;
    df::UserGroupsVisibilitySet visibility = {kGroupId};
    df::MarksIDGroups groups;
    auto ids = make_unique_dp<df::IDCollections>();
    df::UserLinesRenderCollection renderParams;
    auto const tileCenter = tileKey.GetWrappedDataRect().Center();

    for (size_t i = 0; i < std::size(rows); ++i)
    {
      auto const lineId = static_cast<kml::TrackId>(i + 1);
      ids->m_lineIds.push_back(lineId);

      auto params = make_unique_dp<df::UserLineRenderParams>();
      params->m_depthLayer = df::DepthLayer::UserLineLayer;
      params->m_layers.emplace_back(rows[i].m_color, 5.0f, 0.0f);
      double const y = rows[i].m_yInPixels * pixelToGlobalScale;
      params->m_splines.emplace_back(std::vector<m2::PointD>{tileCenter + m2::PointD(-160.0 * pixelToGlobalScale, y),
                                                             tileCenter + m2::PointD(160.0 * pixelToGlobalScale, y)});
      renderParams.emplace(lineId, std::move(params));
    }

    groups.emplace(kGroupId, std::move(ids));
    df::TracksSource source(&visibility);
    source.AddGroup(make_ref(&groups));
    f.AddUserLines(tileKey, source, renderParams);
  });

  QImage const & image = fixture.GetLastImage();
  if (image.isNull())
    return;

  auto const isLight = [](QColor const & color)
  { return color.red() > 225 && color.green() > 225 && color.blue() > 225; };
  auto const isDark = [](QColor const & color)
  { return color.red() < 180 && color.green() < 180 && color.blue() < 180; };

  TEST_GREATER(CountPixels(image, kHeight / 2 - 90, isLight), 0u, ("White arrow frame is invisible on black"));
  TEST_GREATER(CountPixels(image, kHeight / 2 - 30, isDark), 0u, ("Dark arrow outline is invisible on white"));
  TEST_GREATER(CountPixels(image, kHeight / 2 + 30, isLight), 0u, ("White arrow frame is invisible on red"));
  TEST_GREATER(CountPixels(image, kHeight / 2 + 90, isDark), 0u, ("Dark arrow outline is invisible on yellow"));
  TEST_GREATER_OR_EQUAL(GetPixelSpanY(image, kHeight / 2 - 90, isLight), 4, ("Chevron does not use the track width"));

  int constexpr kStartX = kWidth / 2 - 160;
  int constexpr kFinishX = kWidth / 2 + 160;
  TEST_GREATER(CountEndpointPixels(image, kStartX, kHeight / 2 - 90, isLight), 0u,
               ("Start marker is invisible on black"));
  TEST_GREATER(CountEndpointPixels(image, kFinishX, kHeight / 2 - 90, isLight), 0u,
               ("Finish marker is invisible on black"));
  TEST_GREATER(CountEndpointPixels(image, kStartX, kHeight / 2 - 30, isDark), 0u,
               ("Start marker is invisible on white"));
  TEST_GREATER(CountEndpointPixels(image, kFinishX, kHeight / 2 - 30, isDark), 0u,
               ("Finish marker is invisible on white"));
}

UNIT_TEST(TrackCombinedEndpointGpuTest)
{
  uint32_t constexpr kWidth = 240;
  uint32_t constexpr kHeight = 160;
  int constexpr kZoom = 11;
  df::VisualParams::Init(1.0 /* visualScale */, 512 /* tileSize */);
  double const pixelToGlobalScale = df::GetScreenScale(kZoom);
  auto const tileKey = df::GetTileKeyByPoint(mercator::Bounds::FullRect().Center(), kZoom);

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1.0 / pixelToGlobalScale));
  fixture.Render("Combined track endpoint", kWidth, kHeight,
                 [tileKey, pixelToGlobalScale](df::test_support::ShapeTestFixture & f)
  {
    kml::MarkGroupId constexpr kGroupId = 1;
    kml::TrackId constexpr kLineId = 1;
    df::UserGroupsVisibilitySet visibility = {kGroupId};
    df::MarksIDGroups groups;
    auto ids = make_unique_dp<df::IDCollections>();
    ids->m_lineIds.push_back(kLineId);
    groups.emplace(kGroupId, std::move(ids));

    auto const tileCenter = tileKey.GetWrappedDataRect().Center();
    auto params = make_unique_dp<df::UserLineRenderParams>();
    params->m_depthLayer = df::DepthLayer::UserLineLayer;
    params->m_layers.emplace_back(dp::Color::Blue(), 10.0f, 0.0f);
    params->m_splines.emplace_back(
        std::vector<m2::PointD>{tileCenter + m2::PointD(-60.0 * pixelToGlobalScale, 0.0),
                                tileCenter + m2::PointD(60.0 * pixelToGlobalScale, 0.0),
                                tileCenter + m2::PointD(60.0 * pixelToGlobalScale, 40.0 * pixelToGlobalScale),
                                tileCenter + m2::PointD(-55.0 * pixelToGlobalScale, 0.0)});

    df::UserLinesRenderCollection renderParams;
    renderParams.emplace(kLineId, std::move(params));
    df::TracksSource source(&visibility);
    source.AddGroup(make_ref(&groups));
    f.AddUserLines(tileKey, source, renderParams);
  });

  QImage const & image = fixture.GetLastImage();
  if (image.isNull())
    return;

  QColor const center = image.pixelColor(kWidth / 2 - 60, kHeight / 2);
  TEST_GREATER(center.red(), 225, ("Combined start/finish marker has no contrasting center dot"));
  TEST_GREATER(center.green(), 225, ("Combined start/finish marker has no contrasting center dot"));
  TEST_GREATER(center.blue(), 225, ("Combined start/finish marker has no contrasting center dot"));
}

UNIT_TEST(TrackDirectionArrowContainedOnTurnsGpuTest)
{
  using namespace track_direction_gpu_test;

  uint32_t constexpr kWidth = 400;
  uint32_t constexpr kHeight = 300;
  int constexpr kZoom = 11;
  float constexpr kTrackWidth = 5.0f;
  df::VisualParams::Init(1.0 /* visualScale */, 512 /* tileSize */);
  double const pixelToGlobalScale = df::GetScreenScale(kZoom);
  auto const tileKey = df::GetTileKeyByPoint(mercator::Bounds::FullRect().Center(), kZoom);
  std::vector<m2::PointD> const pathInPixels = {{-300.0, 80.0}, {-120.0, 80.0}, {-80.0, 40.0},  {-80.0, -50.0},
                                                {20.0, -50.0},  {60.0, -10.0},  {160.0, -10.0}, {300.0, -10.0}};

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1.0 / pixelToGlobalScale));
  fixture.Render("Track arrows contained on turns", kWidth, kHeight,
                 [tileKey, pixelToGlobalScale, kTrackWidth, &pathInPixels](df::test_support::ShapeTestFixture & f)
  {
    kml::MarkGroupId constexpr kGroupId = 1;
    kml::TrackId constexpr kLineId = 1;
    df::UserGroupsVisibilitySet visibility = {kGroupId};
    df::MarksIDGroups groups;
    auto ids = make_unique_dp<df::IDCollections>();
    ids->m_lineIds.push_back(kLineId);
    groups.emplace(kGroupId, std::move(ids));

    auto const tileCenter = tileKey.GetWrappedDataRect().Center();
    std::vector<m2::PointD> path;
    path.reserve(pathInPixels.size());
    for (auto const & point : pathInPixels)
      path.push_back(tileCenter + point * pixelToGlobalScale);

    auto params = make_unique_dp<df::UserLineRenderParams>();
    params->m_depthLayer = df::DepthLayer::UserLineLayer;
    params->m_layers.emplace_back(dp::Color::White(), kTrackWidth, 0.0f);
    params->m_splines.emplace_back(std::move(path));

    df::UserLinesRenderCollection renderParams;
    renderParams.emplace(kLineId, std::move(params));
    df::TracksSource source(&visibility);
    source.AddGroup(make_ref(&groups));
    f.AddUserLines(tileKey, source, renderParams);
  });

  QImage const & image = fixture.GetLastImage();
  if (image.isNull())
    return;

  std::vector<m2::PointD> screenPath;
  screenPath.reserve(pathInPixels.size());
  for (auto const & point : pathInPixels)
    screenPath.emplace_back(kWidth / 2.0 + point.x, kHeight / 2.0 - point.y);

  uint32_t darkPixels = 0;
  double constexpr kAntialiasingTolerance = 1.0;
  for (int y = 0; y < image.height(); ++y)
  {
    for (int x = 0; x < image.width(); ++x)
    {
      QColor const color = image.pixelColor(x, y);
      if (color.red() >= 180 || color.green() >= 180 || color.blue() >= 180)
        continue;

      ++darkPixels;
      double const distance = DistanceToPolyline({static_cast<double>(x), static_cast<double>(y)}, screenPath);
      TEST_LESS_OR_EQUAL(distance, 0.5 * kTrackWidth + kAntialiasingTolerance,
                         ("Arrow frame escapes the track at", x, y, distance));
    }
  }
  TEST_GREATER(darkPixels, 0u, ("No direction arrows were rendered"));
}

UNIT_TEST(TrackDirectionArrowVisibleOnGentleCurveGpuTest)
{
  uint32_t constexpr kWidth = 400;
  uint32_t constexpr kHeight = 200;
  int constexpr kZoom = 11;
  df::VisualParams::Init(1.0 /* visualScale */, 512 /* tileSize */);
  double const pixelToGlobalScale = df::GetScreenScale(kZoom);
  auto const tileKey = df::GetTileKeyByPoint(mercator::Bounds::FullRect().Center(), kZoom);

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1.0 / pixelToGlobalScale));
  fixture.Render("Track arrows visible on gentle curve", kWidth, kHeight,
                 [tileKey, pixelToGlobalScale](df::test_support::ShapeTestFixture & f)
  {
    kml::MarkGroupId constexpr kGroupId = 1;
    kml::TrackId constexpr kLineId = 1;
    df::UserGroupsVisibilitySet visibility = {kGroupId};
    df::MarksIDGroups groups;
    auto ids = make_unique_dp<df::IDCollections>();
    ids->m_lineIds.push_back(kLineId);
    groups.emplace(kGroupId, std::move(ids));

    auto const tileCenter = tileKey.GetWrappedDataRect().Center();
    std::vector<m2::PointD> path;
    for (int x = -300; x <= 300; x += 4)
    {
      double const y = 20.0 * std::sin(x / 120.0);
      path.push_back(tileCenter + m2::PointD(x, y) * pixelToGlobalScale);
    }

    auto params = make_unique_dp<df::UserLineRenderParams>();
    params->m_depthLayer = df::DepthLayer::UserLineLayer;
    params->m_layers.emplace_back(dp::Color::White(), 5.0f, 0.0f);
    params->m_splines.emplace_back(std::move(path));

    df::UserLinesRenderCollection renderParams;
    renderParams.emplace(kLineId, std::move(params));
    df::TracksSource source(&visibility);
    source.AddGroup(make_ref(&groups));
    f.AddUserLines(tileKey, source, renderParams);
  });

  QImage const & image = fixture.GetLastImage();
  if (image.isNull())
    return;

  uint32_t darkPixels = 0;
  for (int y = 0; y < image.height(); ++y)
  {
    for (int x = 0; x < image.width(); ++x)
    {
      QColor const color = image.pixelColor(x, y);
      if (color.red() < 180 && color.green() < 180 && color.blue() < 180)
        ++darkPixels;
    }
  }
  TEST_GREATER(darkPixels, 0u, ("Gentle curve rejected every direction chevron"));
}
