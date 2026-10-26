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

struct TestTrack
{
  std::vector<m2::PointD> m_pathInPixels;
  std::vector<df::LineLayer> m_layers;
  bool m_hasDirection = true;
};

QImage RenderTracks(char const * title, uint32_t width, uint32_t height, std::vector<TestTrack> const & tracks,
                    int zoom = 11)
{
  df::VisualParams::Init(1.0 /* visualScale */, 512 /* tileSize */);
  auto const tileKey = df::GetTileKeyByPoint(mercator::Bounds::FullRect().Center(), zoom);
  double const scale = df::GetScreenScale(zoom);

  df::test_support::ShapeTestFixture fixture;
  fixture.SetWorldToPixelScale(static_cast<float>(1.0 / scale));
  fixture.Render(title, width, height, [&](df::test_support::ShapeTestFixture & f)
  {
    kml::MarkGroupId constexpr kGroupId = 1;
    df::UserGroupsVisibilitySet visibility = {kGroupId};
    df::MarksIDGroups groups;
    auto ids = make_unique_dp<df::IDCollections>();
    df::UserLinesRenderCollection renderParams;
    auto const center = tileKey.GetWrappedDataRect().Center();

    for (size_t i = 0; i < tracks.size(); ++i)
    {
      auto const lineId = static_cast<kml::TrackId>(i + 1);
      ids->m_lineIds.push_back(lineId);
      auto const & track = tracks[i];
      auto params = make_unique_dp<df::UserLineRenderParams>();
      params->m_layers = track.m_layers;
      params->m_hasDirection = track.m_hasDirection;
      std::vector<m2::PointD> path;
      path.reserve(track.m_pathInPixels.size());
      for (auto const & point : track.m_pathInPixels)
        path.push_back(center + point * scale);
      params->m_splines.emplace_back(std::move(path));
      renderParams.emplace(lineId, std::move(params));
    }

    groups.emplace(kGroupId, std::move(ids));
    df::TracksSource source(&visibility);
    source.AddGroup(make_ref(&groups));
    f.AddUserLines(tileKey, source, renderParams);
  });
  return fixture.GetLastImage();
}
}  // namespace track_direction_gpu_test

UNIT_TEST(TrackDirectionArrowsAvoidEndpointsGpuTest)
{
  using namespace track_direction_gpu_test;

  uint32_t constexpr kWidth = 600;
  uint32_t constexpr kHeight = 480;
  std::vector<std::vector<m2::PointD>> const paths = {{{-160, 200}, {84, 200}},
                                                      {{-160, 150}, {86, 150}},
                                                      {{-160, 100}, {130, 100}},
                                                      {{-160, 10}, {-75, 10}, {-75, 47}, {-160, 47}, {-160, 10}},
                                                      {{-160, -80}, {-75, -80}, {-75, -43}, {-160, -43}, {-160, -91}},
                                                      {{-400, -160}, {-156, -160}}};

  auto const render = [&](float lineWidth)
  {
    std::vector<TestTrack> tracks;
    for (auto const & path : paths)
      tracks.push_back({path, {{dp::Color::Blue(), lineWidth, 0.0f}}});
    return RenderTracks("Track arrows keep endpoint markers clear", kWidth, kHeight, tracks);
  };

  auto const image = render(5.0f);
  // This width suppresses arrows while leaving endpoint marker geometry unchanged.
  auto const reference = render(2.0f);
  if (image.isNull() || reference.isNull())
    return;

  for (size_t i = 0; i < paths.size(); ++i)
  {
    bool const combined = i == 3 || i == 4;
    auto const & endpoint = combined ? paths[i].front() : paths[i].back();
    int const centerX = static_cast<int>(kWidth / 2 + endpoint.x);
    int const centerY = static_cast<int>(kHeight / 2 - endpoint.y);
    for (int dy = -4; dy < 4; ++dy)
      for (int dx = -4; dx < 4; ++dx)
        TEST_EQUAL(image.pixel(centerX + dx, centerY + dy), reference.pixel(centerX + dx, centerY + dy),
                   ("Arrow covers endpoint marker", i, dx, dy));
  }

  uint32_t arrowPixels = 0;
  for (int x = static_cast<int>(kWidth) / 2 - 140; x < static_cast<int>(kWidth) / 2 + 60; ++x)
  {
    auto const color = image.pixelColor(x, kHeight / 2 - 200);
    if (color.red() > 100 && color.green() > 100)
      ++arrowPixels;
  }
  TEST_GREATER(arrowPixels, 0u, ("Direction arrows disappeared away from endpoint markers"));
}

UNIT_TEST(TrackEndpointUsesTopLayerColorGpuTest)
{
  uint32_t constexpr kWidth = 240;
  uint32_t constexpr kHeight = 160;
  auto const image = track_direction_gpu_test::RenderTracks(
      "Track endpoints use the top layer color", kWidth, kHeight,
      {{{{-80, 0}, {80, 0}}, {{dp::Color::Blue(), 5.0f, 1.0f}, {dp::Color::Red(), 9.0f, 0.0f}}}});
  if (image.isNull())
    return;

  auto const expected = QColor(Qt::blue).rgba();
  TEST_EQUAL(image.pixel(kWidth / 2 - 80, kHeight / 2), expected, ("Start marker uses the wrong layer"));
  TEST_EQUAL(image.pixel(kWidth / 2 + 80, kHeight / 2), expected, ("Finish marker uses the wrong layer"));
}

UNIT_TEST(TrackDirectionArrowContrastGpuTest)
{
  using namespace track_direction_gpu_test;

  uint32_t constexpr kWidth = 400;
  uint32_t constexpr kHeight = 240;
  auto const image = RenderTracks("Track direction arrow contrast", kWidth, kHeight,
                                  {{{{-160, 90}, {160, 90}}, {{dp::Color::Black(), 5.0f, 0.0f}}},
                                   {{{-160, 30}, {160, 30}}, {{dp::Color::White(), 5.0f, 0.0f}}},
                                   {{{-160, -30}, {160, -30}}, {{dp::Color::Red(), 5.0f, 0.0f}}},
                                   {{{-160, -90}, {160, -90}}, {{dp::Color::Yellow(), 5.0f, 0.0f}}}});
  if (image.isNull())
    return;

  auto const isLight = [](QColor const & color)
  { return color.red() > 225 && color.green() > 225 && color.blue() > 225; };
  auto const isDark = [](QColor const & color)
  { return color.red() < 180 && color.green() < 180 && color.blue() < 180; };

  TEST_GREATER(CountPixels(image, kHeight / 2 - 90, isLight), 0u, ("White chevron is invisible on black"));
  TEST_GREATER(CountPixels(image, kHeight / 2 - 30, isDark), 0u, ("Dark chevron is invisible on white"));
  TEST_GREATER(CountPixels(image, kHeight / 2 + 30, isLight), 0u, ("White chevron is invisible on red"));
  TEST_GREATER(CountPixels(image, kHeight / 2 + 90, isDark), 0u, ("Dark chevron is invisible on yellow"));
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
  auto const image = track_direction_gpu_test::RenderTracks(
      "Combined track endpoint", kWidth, kHeight,
      {{{{-60, 0}, {60, 0}, {60, 40}, {-55, 0}}, {{dp::Color::Blue(), 10.0f, 0.0f}}}});
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
  float constexpr kTrackWidth = 5.0f;
  std::vector<m2::PointD> const pathInPixels = {{-300.0, 80.0}, {-120.0, 80.0}, {-80.0, 40.0},  {-80.0, -50.0},
                                                {20.0, -50.0},  {60.0, -10.0},  {160.0, -10.0}, {300.0, -10.0}};

  auto const image = RenderTracks("Track arrows contained on turns", kWidth, kHeight,
                                  {{pathInPixels, {{dp::Color::White(), kTrackWidth, 0.0f}}}});
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
                         ("Chevron escapes the track at", x, y, distance));
    }
  }
  TEST_GREATER(darkPixels, 0u, ("No direction arrows were rendered"));
}

UNIT_TEST(TrackDirectionArrowVisibleOnGentleCurveGpuTest)
{
  uint32_t constexpr kWidth = 400;
  uint32_t constexpr kHeight = 200;
  std::vector<m2::PointD> path;
  for (int x = -300; x <= 300; x += 4)
    path.emplace_back(x, 20.0 * std::sin(x / 120.0));
  auto const image = track_direction_gpu_test::RenderTracks("Track arrows visible on gentle curve", kWidth, kHeight,
                                                            {{path, {{dp::Color::White(), 5.0f, 0.0f}}}});
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

UNIT_TEST(TrackDirectionArrowsAvoidOtherTrackEndpointsGpuTest)
{
  using namespace track_direction_gpu_test;

  uint32_t constexpr kWidth = 300;
  uint32_t constexpr kHeight = 400;
  int constexpr kCenterX = kWidth / 2;
  int constexpr kCenterY = kHeight / 2;
  auto const render = [](float crossingWidth)
  {
    return RenderTracks("Track arrows keep other tracks' endpoint markers clear", kWidth, kHeight,
                        {{{{-100, 0}, {0, 0}}, {{dp::Color::Blue(), 2.0f, 0.0f}}},
                         {{{0, -144}, {0, 160}}, {{dp::Color::Red(), crossingWidth, 0.0f}}}});
  };

  auto const image = render(5.0f);
  auto const reference = render(2.0f);
  if (image.isNull() || reference.isNull())
    return;

  for (int dy = -4; dy < 4; ++dy)
    for (int dx = -4; dx < 4; ++dx)
      TEST_EQUAL(image.pixel(kCenterX + dx, kCenterY + dy), reference.pixel(kCenterX + dx, kCenterY + dy),
                 ("Another track's arrow covers the finish marker", dx, dy));

  uint32_t arrowPixels = 0;
  for (int y = kCenterY + 90; y < kCenterY + 102; ++y)
    for (int x = kCenterX - 2; x < kCenterX + 2; ++x)
    {
      auto const color = image.pixelColor(x, y);
      if (color.green() > 100 && color.blue() > 100)
        ++arrowPixels;
    }
  TEST_GREATER(arrowPixels, 0u, ("Direction arrows disappeared away from the other track's marker"));
}

UNIT_TEST(TrackWithoutDirectionHasNoDecorationsGpuTest)
{
  uint32_t constexpr kWidth = 240;
  uint32_t constexpr kHeight = 160;
  int constexpr kCenterX = kWidth / 2;
  int constexpr kCenterY = kHeight / 2;
  for (int const zoom : {8, 11})
  {
    auto const image = track_direction_gpu_test::RenderTracks(
        "Tracks without direction have no arrows or endpoint markers", kWidth, kHeight,
        {{{{-80, 0}, {80, 0}}, {{dp::Color::Blue(), 5.0f, 0.0f}}, false /* hasDirection */}}, zoom);
    if (image.isNull())
      return;

    auto const white = QColor(Qt::white).rgba();
    TEST_EQUAL(image.pixel(kCenterX - 80, kCenterY + 4), white, ("Unexpected start marker", zoom));
    TEST_EQUAL(image.pixel(kCenterX + 80, kCenterY + 4), white, ("Unexpected finish marker", zoom));
    for (int x = kCenterX - 75; x < kCenterX + 75; ++x)
      TEST_EQUAL(image.pixel(x, kCenterY), QColor(Qt::blue).rgba(), ("Unexpected direction arrow", zoom, x));
  }
}
