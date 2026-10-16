#include "testing/testing.hpp"

#include "map/gps_track_collection.hpp"
#include "map/gps_track_filter.hpp"
#include "map/track.hpp"
#include "map/track_elevation.hpp"

#include "coding/file_reader.hpp"
#include "geometry/mercator.hpp"
#include "geometry/simplification.hpp"
#include "kml/serdes_gpx.hpp"
#include "platform/platform.hpp"

#include <array>
#include <cmath>
#include <random>

namespace track_elevation_tests
{
using Point = TrackElevation::Point;

void CheckTotals(TrackElevation::Totals const & actual, TrackElevation::Totals const & expected)
{
  TEST_EQUAL(actual.m_ascent, expected.m_ascent, ());
  TEST_EQUAL(actual.m_descent, expected.m_descent, ());
  TEST_EQUAL(actual.m_smallAscent, expected.m_smallAscent, ());
  TEST_EQUAL(actual.m_smallDescent, expected.m_smallDescent, ());
  TEST_EQUAL(actual.m_min, expected.m_min, ());
  TEST_EQUAL(actual.m_max, expected.m_max, ());
}

void CheckPoints(std::span<Point const> actual, std::span<Point const> expected)
{
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < std::min(actual.size(), expected.size()); ++i)
  {
    TEST_EQUAL(actual[i].m_distance, expected[i].m_distance, (i));
    TEST_EQUAL(actual[i].m_altitude, expected[i].m_altitude, (i));
  }
}

UNIT_TEST(TrackElevation_PrefixesAndDeferredProfiles)
{
  std::mt19937 rng(13233);
  for (size_t kind = 0; kind < 6; ++kind)
  {
    TrackElevation live, graph;
    std::vector<Point> input, graphPrefix, livePrefix;
    size_t graphCursor = 0;
    double distance = 0;
    for (size_t i = 0; i < 800; ++i)
    {
      distance += kind == 5 ? rng() % 30 : 12;
      int altitude = 100;
      switch (kind)
      {
      case 0: altitude += int(i % 3) - 1; break;
      case 1: altitude += i < 700 ? int(i % 3) - 1 : int(i - 700) / 2; break;
      case 2: altitude += int(30 * std::sin(i * 0.005)) + int(i % 5); break;
      case 3: altitude += int(std::min(i, size_t{800} - i)) / 10; break;
      case 4: altitude += i % 2 ? 150 : -150; break;
      case 5: altitude += int(rng() % 200) - 150; break;
      }
      Point const point{distance, static_cast<geometry::Altitude>(altitude)};
      input.push_back(point);
      live.Add(point, [&](auto points) { livePrefix.insert(livePrefix.end(), points.begin(), points.end()); });
      // Queries must not seal a provisional tail or change future window alignment.
      auto const totals = live.GetTotals();
      CheckTotals(live.GetTotals(), totals);
      TEST_LESS_OR_EQUAL(live.GetTail().size(), TrackElevation::kMaxTailPoints, ());
      bool const boundary = i % TrackElevation::kWindowIntervals <= 2 || i % TrackElevation::kWindowIntervals >= 254;
      if (i % 173 != 0 && i != 799 && !boundary)
        continue;

      TrackElevation replay;
      for (auto p : input)
        replay.Add(p);
      CheckTotals(totals, replay.GetTotals());

      // Graph work catches up only when explicitly requested after a hidden interval.
      for (; graphCursor < input.size(); ++graphCursor)
        graph.Add(input[graphCursor],
                  [&](auto points) { graphPrefix.insert(graphPrefix.end(), points.begin(), points.end()); });
      CheckTotals(graph.GetTotals(), totals);
      CheckPoints(graphPrefix, livePrefix);
      CheckPoints(graph.GetTail(), live.GetTail());

      auto processed = livePrefix;
      auto const tail = live.GetTail().subspan(live.HasPrefix() ? 1 : 0);
      processed.insert(processed.end(), tail.begin(), tail.end());
      auto const info = ElevationInfo(std::vector<ElevationInfo::Points>{processed}).CalculateAltitudesInfo(10);
      TEST_EQUAL(totals.m_ascent, info.m_totalAscentFiltered, ());
      TEST_EQUAL(totals.m_descent, info.m_totalDescentFiltered, ());
      TEST_EQUAL(totals.m_min, info.m_minAltitude, ());
      TEST_EQUAL(totals.m_max, info.m_maxAltitude, ());
    }
  }
}

UNIT_TEST(TrackElevation_NoiseSmallHillsAndLongOutliers)
{
  TrackElevation quiet;
  for (size_t i = 0; i < 20000; ++i)
    quiet.Add({double(i) * 12, static_cast<geometry::Altitude>(100 + int(i % 3) - 1)});
  TrackStatistics stats;
  quiet.GetTotals().Apply(stats);
  TEST_EQUAL(stats.m_ascent, 0, ());
  TEST_EQUAL(stats.m_descent, 0, ());

  for (size_t outliers : {size_t{16}, size_t{17}, size_t{10000}})
  {
    TrackElevation elevation;
    elevation.Add({0, 100});
    for (size_t i = 1; i <= outliers; ++i)
      elevation.Add({double(i), static_cast<geometry::Altitude>(i % 2 ? 200 : 0)});
    elevation.Add({double(outliers + 1), 100});
    elevation.Add({double(outliers + 2), 100});
    TrackStatistics result;
    elevation.GetTotals().Apply(result);
    TEST_EQUAL(result.m_ascent, 0, (outliers));
    TEST_EQUAL(result.m_descent, 0, (outliers));
    TEST_EQUAL(result.m_minElevation, 100, (outliers));
    TEST_EQUAL(result.m_maxElevation, 100, (outliers));
  }

  TrackElevation small;
  for (auto point : {Point{0, -30}, Point{12, -24}, Point{24, -30}})
    small.Add(point);
  small.GetTotals().Apply(stats);
  TEST_EQUAL(stats.m_ascent, 6, ());
  TEST_EQUAL(stats.m_descent, 6, ());
}

UNIT_TEST(TrackElevation_SimplifierScratchReuse)
{
  std::mt19937 rng(42);
  std::array<simpl::SimplifyOptimalRes, 257> scratch;
  for (size_t test = 0; test < 100; ++test)
  {
    std::vector<m2::PointD> input, expected, actual;
    size_t const count = 3 + rng() % 255;
    for (size_t i = 0; i < count; ++i)
      input.emplace_back(12.0 * i, int(rng() % 100));
    m2::SquaredDistanceFromSegmentToPoint distance;
    SimplifyDefault(input.begin(), input.end(), 2.0, expected);
    SimplifyNearOptimal(20, input.begin(), input.end(), 2.0, distance, AccumulateSkipSmallTrg(distance, actual, 2.0),
                        scratch);
    TEST_EQUAL(actual, expected, (test));
  }
}

UNIT_TEST(TrackElevation_GpxCompatibility)
{
  struct Case
  {
    char const * m_file;
    double m_ascent, m_descent, m_recordingAscent, m_recordingDescent;
  };
  for (auto const & test :
       {Case{"Peak 4120.20260209", 532, 530, 568, 572}, Case{"osm_track", 12, 19, 12, 19},
        Case{"Solden (Tirol) to Merano (South Tirol)", 1407, 2436, 1393, 2422}, Case{"gpx_studio", 54, 50, 31, 34},
        Case{"transalpes-2026-preparation", 1610, 925, 1576, 891},
        Case{"East River Trail to Allen Mountain", 828, 64, 829, 65}})
  {
    kml::FileData file;
    kml::DeserializerGpx(file).Deserialize(
        FileReader(GetPlatform().TestsDataPathForFile(std::string("test_data/gpx/") + test.m_file + ".gpx")));
    TEST_EQUAL(file.m_tracksData.size(), 1, (test.m_file));
    auto data = file.m_tracksData.front();
    data.m_id = 42;
    auto const saved = Track(kml::TrackData(data)).GetStatistics();
    TEST_EQUAL(saved.m_ascent, test.m_ascent, (test.m_file));
    TEST_EQUAL(saved.m_descent, test.m_descent, (test.m_file));

    auto const & line = data.m_geometry.m_lines.front();
    std::vector<location::GpsInfo> raw, accepted;
    for (size_t i = 0; i < line.size(); ++i)
    {
      location::GpsInfo gps;
      auto const ll = line[i].ToLatLon();
      gps.m_source = location::EAppleNative;
      gps.m_latitude = ll.m_lat;
      gps.m_longitude = ll.m_lon;
      gps.m_altitude = line[i].GetAltitude();
      gps.m_timestamp = 1000000 + i * 10;
      gps.m_horizontalAccuracy = gps.m_verticalAccuracy = 1;
      gps.m_speed = i == 0 ? 0 : mercator::DistanceOnEarth(line[i - 1].GetPoint(), line[i].GetPoint()) / 10;
      raw.push_back(gps);
    }
    GpsTrackFilter filter;
    filter.Process(raw, accepted);
    filter.Finalize(accepted);
    GpsTrackCollection collection;
    collection.Add(accepted);
    auto const live = collection.GetTrackStatistics();
    TEST_EQUAL(live.m_ascent, test.m_recordingAscent, (test.m_file));
    TEST_EQUAL(live.m_descent, test.m_recordingDescent, (test.m_file));
    data.m_geometry.m_lines = {{}};
    data.m_geometry.m_timestamps = {{}};
    for (auto const & gps : accepted)
    {
      data.m_geometry.m_lines.front().emplace_back(mercator::FromLatLon(gps.m_latitude, gps.m_longitude),
                                                   static_cast<geometry::Altitude>(gps.m_altitude));
      data.m_geometry.m_timestamps.front().push_back(gps.m_timestamp);
    }
    auto const recorded = Track(std::move(data)).GetStatistics();
    TEST_EQUAL(live.m_ascent, recorded.m_ascent, (test.m_file));
    TEST_EQUAL(live.m_descent, recorded.m_descent, (test.m_file));
    TEST_EQUAL(live.m_minElevation, recorded.m_minElevation, (test.m_file));
    TEST_EQUAL(live.m_maxElevation, recorded.m_maxElevation, (test.m_file));
    TEST_EQUAL(live.m_length, recorded.m_length, (test.m_file));
    TEST_EQUAL(live.m_duration, recorded.m_duration, (test.m_file));
  }
}

UNIT_TEST(TrackElevation_LineBoundariesAndGeometryChanges)
{
  kml::TrackData data;
  data.m_id = 42;
  data.m_layers.emplace_back();
  for (auto const & altitudes : {std::vector<int>{0, 6, 0}, std::vector<int>{100, 130}})
  {
    auto & line = data.m_geometry.m_lines.emplace_back();
    data.m_geometry.m_timestamps.emplace_back();
    for (auto altitude : altitudes)
      line.emplace_back(mercator::FromLatLon(45, line.size() * 0.0003), static_cast<geometry::Altitude>(altitude));
  }
  Track saved{kml::TrackData(data)};
  auto const first = saved.GetStatistics();
  TEST_EQUAL(first.m_ascent, 30, ());
  TEST_EQUAL(first.m_descent, 0, ());  // Main totals select the policy for the whole track.
  TEST_EQUAL(first.m_minElevation, 0, ());
  TEST_EQUAL(first.m_maxElevation, 130, ());
  TEST_EQUAL(saved.GetElevationInfo()->GetLines().size(), 2, ());
  saved.SetName("renamed");
  saved.SetVisibility(false);
  TEST_EQUAL(saved.GetStatistics().m_ascent, first.m_ascent, ());
  for (auto & line : data.m_geometry.m_lines)
    for (auto & point : line)
      point.SetAltitude(geometry::kDefaultAltitudeMeters);
  saved.SetData(data);
  TEST(!saved.GetElevationInfo(), ());
  TEST_EQUAL(saved.GetStatistics().m_ascent, 0, ());
  TEST_EQUAL(saved.GetStatistics().m_maxElevation, 0, ());
  data.m_geometry.m_lines.back().back().SetAltitude(200);
  saved.SetData(data);
  TEST(saved.GetElevationInfo(), ());
  TEST_EQUAL(saved.GetStatistics().m_maxElevation, 200, ());
  data.m_geometry.m_lines.front().front().SetAltitude(geometry::kInvalidAltitude);
  saved.SetData(data);
  TEST(!saved.GetElevationInfo(), ());
  TEST_EQUAL(saved.GetStatistics().m_maxElevation, 0, ());
}
}  // namespace track_elevation_tests
