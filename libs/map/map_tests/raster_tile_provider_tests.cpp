#include "testing/testing.hpp"

#include "map/raster_tile_provider.hpp"

#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace
{
using RTP = RasterTileProvider;
using namespace std::chrono_literals;

// Reference: the standard web-mercator (XYZ / "slippy map") tile index for a geographic point.
void StandardWebTile(double lat, double lon, int z, int & x, int & y)
{
  double const n = std::ldexp(1.0, z);  // 2^z
  x = static_cast<int>(std::floor((lon + 180.0) / 360.0 * n));
  double const latRad = lat * math::pi / 180.0;
  y = static_cast<int>(std::floor((1.0 - std::asinh(std::tan(latRad)) / math::pi) / 2.0 * n));
}

// The OM tile that contains the point at the given OM zoomLevel (m_x/m_y in the 2^(zoom-1) grid,
// matching m_zoomLevel — which is exactly the contract ToSourceTile relies on).
df::TileKey OmTileAt(double lat, double lon, int omZoom)
{
  return df::GetTileKeyByPoint(mercator::FromLatLon(lat, lon), omZoom);
}

platform::HttpClient::Result FailedDownload()
{
  platform::HttpClient::Result result;
  result.m_errorCode = 500;
  return result;
}

class ControlledDownloader
{
public:
  RTP::TDownloadFn GetDownloadFn()
  {
    return [this](std::string const & url, platform::HttpClient::CompletionHandler handler)
    {
      auto request = std::make_shared<Request>();
      request->m_url = url;
      request->m_handler = std::move(handler);
      {
        std::lock_guard lock(m_mutex);
        m_requests.push_back(request);
      }
      m_cv.notify_all();

      return [this, request]
      {
        {
          std::lock_guard lock(m_mutex);
          request->m_cancelled = true;
        }
        m_cv.notify_all();
      };
    };
  }

  bool WaitForRequests(size_t count)
  {
    std::unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, 5s, [this, count] { return m_requests.size() >= count; });
  }

  bool WaitCancelled(size_t index)
  {
    std::unique_lock lock(m_mutex);
    return m_cv.wait_for(lock, 5s,
                         [this, index] { return index < m_requests.size() && m_requests[index]->m_cancelled; });
  }

  void Complete(size_t index, platform::HttpClient::Result result)
  {
    platform::HttpClient::CompletionHandler handler;
    {
      std::lock_guard lock(m_mutex);
      TEST_LESS(index, m_requests.size(), ());
      handler = m_requests[index]->m_handler;
    }
    // Invoke even after cancellation: a real transport callback may already be queued when Cancel runs.
    handler(std::move(result));
  }

private:
  struct Request
  {
    std::string m_url;
    platform::HttpClient::CompletionHandler m_handler;
    bool m_cancelled = false;
  };

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::vector<std::shared_ptr<Request>> m_requests;
};

class ReadyRecorder
{
public:
  RTP::TReadyFn GetReadyFn()
  {
    return [this](df::TileKey const & key, dp::BackgroundMode, std::string const &, uint32_t, uint32_t,
                  m2::RectF const &, std::vector<uint8_t> &&)
    {
      std::lock_guard lock(m_mutex);
      ++m_deliveries[key];
    };
  }

  size_t Count() const
  {
    std::lock_guard lock(m_mutex);
    size_t count = 0;
    for (auto const & [key, deliveries] : m_deliveries)
      count += deliveries;
    return count;
  }

  size_t Count(df::TileKey const & key) const
  {
    std::lock_guard lock(m_mutex);
    auto const it = m_deliveries.find(key);
    return it == m_deliveries.end() ? 0 : it->second;
  }

private:
  mutable std::mutex m_mutex;
  std::unordered_map<df::TileKey, size_t> m_deliveries;
};

RTP::Params TestParams(std::string cacheSubdir)
{
  RTP::Params params;
  params.m_urlTemplate = "https://tiles.test/{z}/{x}/{y}.png";
  params.m_cacheSubdir = std::move(cacheSubdir);
  return params;
}
}  // namespace

// A correctly-built OM tile (grid resolution matches m_zoomLevel) must map to the geographically
// correct web-mercator tile across the whole zoom range, for points in every hemisphere.
UNIT_TEST(RasterTileProvider_ToSourceTile_MatchesWebMercator)
{
  double const pts[][2] = {
      {50.584312, 33.724862},  // the "z18 desert tile" bug report point
      {0.1, 0.1},
      {-33.8688, 151.2093},
      {64.5, -147.0},
      {-54.8, -68.3},
  };
  int const kMaxZoom = 30;  // high enough that no tile is over-zoomed: each maps to a full web tile

  for (auto const & p : pts)
  {
    double const lat = p[0], lon = p[1];
    for (int omZoom = 2; omZoom <= 21; ++omZoom)
    {
      auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, omZoom), 0 /* minZoom */, kMaxZoom);
      TEST(src.IsValid(), (lat, lon, omZoom));

      int const z = omZoom - 1;
      int ex = 0, ey = 0;
      StandardWebTile(lat, lon, z, ex, ey);
      TEST_EQUAL(src.m_z, z, (lat, lon, omZoom));
      TEST_EQUAL(src.m_x, ex, (lat, lon, omZoom));
      TEST_EQUAL(src.m_y, ey, (lat, lon, omZoom));
      // Full tile, no sub-rect when not over-zoomed.
      TEST_EQUAL(src.m_rect, m2::RectF(0.0f, 0.0f, 1.0f, 1.0f), (lat, lon, omZoom));
    }
  }
}

// Regression for the "z18 desert tile" bug, with the exact browser-verified web tiles. The bug was
// upstream (the OM coverage was clamped to GetUpperScale()=17, so m_x/m_y lagged m_zoomLevel above
// zoom 17); these values lock in the conversion contract that the fix restores.
UNIT_TEST(RasterTileProvider_ToSourceTile_BugReportCoords)
{
  double const lat = 50.584312, lon = 33.724862;
  struct
  {
    int omZoom, z, x, y;
  } const expected[] = {
      {17, 16, 38907, 22059},
      {18, 17, 77814, 44119},
      {19, 18, 155629, 88238},
  };

  for (auto const & e : expected)
  {
    auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, e.omZoom), 0 /* minZoom */, 30 /* maxZoom */);
    TEST(src.IsValid(), (e.omZoom));
    TEST_EQUAL(src.m_z, e.z, (e.omZoom));
    TEST_EQUAL(src.m_x, e.x, (e.omZoom));
    TEST_EQUAL(src.m_y, e.y, (e.omZoom));
  }
}

// Beyond maxZoom the source is the maxZoom ancestor tile plus a sub-rect within it.
UNIT_TEST(RasterTileProvider_ToSourceTile_OverZoom)
{
  int const kMaxZoom = 19;
  double const lat = 50.584312, lon = 33.724862;
  int const omZoom = 22;  // web z = 21, k = 21 - 19 = 2, so a 4x4 grid of children share the ancestor

  auto const src = RTP::ToSourceTile(OmTileAt(lat, lon, omZoom), 0 /* minZoom */, kMaxZoom);
  TEST(src.IsValid(), ());
  TEST_EQUAL(src.m_z, kMaxZoom, ());

  int ax = 0, ay = 0;
  StandardWebTile(lat, lon, kMaxZoom, ax, ay);  // the maxZoom ancestor contains the point
  TEST_EQUAL(src.m_x, ax, ());
  TEST_EQUAL(src.m_y, ay, ());

  // The sub-rect is one of the 4x4 = 0.25-sized cells, fully inside the unit square.
  TEST(src.m_rect.minX() >= 0.0f && src.m_rect.maxX() <= 1.0f, (src.m_rect));
  TEST(src.m_rect.minY() >= 0.0f && src.m_rect.maxY() <= 1.0f, (src.m_rect));
  TEST_ALMOST_EQUAL_ABS(src.m_rect.SizeX(), 0.25f, 1e-5f, ());
  TEST_ALMOST_EQUAL_ABS(src.m_rect.SizeY(), 0.25f, 1e-5f, ());
}

// Tiles below the configured minimum source zoom are skipped (reported invalid, never requested).
UNIT_TEST(RasterTileProvider_ToSourceTile_BelowMinZoomInvalid)
{
  // OM zoom 6 -> web z5, below minZoom 7.
  auto const src = RTP::ToSourceTile(OmTileAt(50.584312, 33.724862, 6), 7 /* minZoom */, 19 /* maxZoom */);
  TEST(!src.IsValid(), ());
}

// The antimeridian copies (OM x outside the base [-n/2, n/2) range) wrap onto valid [0, n) web tiles.
UNIT_TEST(RasterTileProvider_ToSourceTile_AntimeridianWrap)
{
  int const omZoom = 5;  // web z4, n = 16
  int const n = 1 << (omZoom - 1);
  df::TileKey const base(3, 2, static_cast<uint8_t>(omZoom));
  auto const src0 = RTP::ToSourceTile(base, 0, 30);
  // A whole-world shift east must land on the same web tile.
  df::TileKey const shifted(3 + n, 2, static_cast<uint8_t>(omZoom));
  auto const src1 = RTP::ToSourceTile(shifted, 0, 30);
  TEST(src0.IsValid() && src1.IsValid(), ());
  TEST_EQUAL(src0.m_x, src1.m_x, ());
  TEST_EQUAL(src0.m_y, src1.m_y, ());
  TEST(src0.m_x >= 0 && src0.m_x < n, (src0.m_x, n));
}

UNIT_TEST(RasterTileProvider_CancelStopsTransportAndSuppressesLateCompletion)
{
  Platform::ThreadRunner runner;
  std::string const cacheSubdir = "raster_tile_provider_cancel_test";
  platform::tests_support::ScopedDirCleanup cleanup(base::JoinPath(GetPlatform().WritableDir(), cacheSubdir));
  ControlledDownloader downloader;
  ReadyRecorder ready;
  RTP provider(TestParams(cacheSubdir), ready.GetReadyFn(), downloader.GetDownloadFn());
  auto const key = OmTileAt(50.584312, 33.724862, 10);

  TEST(provider.RequestTile(key, dp::BackgroundMode::Satellite), ());
  TEST(downloader.WaitForRequests(1), ());
  provider.CancelTile(key, dp::BackgroundMode::Satellite);
  TEST(downloader.WaitCancelled(0), ());

  downloader.Complete(0, FailedDownload());
  TEST_EQUAL(ready.Count(), size_t{0}, ());
}

UNIT_TEST(RasterTileProvider_CancelThenRerequestIgnoresOldCompletion)
{
  Platform::ThreadRunner runner;
  std::string const cacheSubdir = "raster_tile_provider_rerequest_test";
  platform::tests_support::ScopedDirCleanup cleanup(base::JoinPath(GetPlatform().WritableDir(), cacheSubdir));
  ControlledDownloader downloader;
  ReadyRecorder ready;
  RTP provider(TestParams(cacheSubdir), ready.GetReadyFn(), downloader.GetDownloadFn());
  auto const key = OmTileAt(50.584312, 33.724862, 10);

  TEST(provider.RequestTile(key, dp::BackgroundMode::Satellite), ());
  TEST(downloader.WaitForRequests(1), ());
  provider.CancelTile(key, dp::BackgroundMode::Satellite);
  TEST(downloader.WaitCancelled(0), ());

  TEST(provider.RequestTile(key, dp::BackgroundMode::Satellite), ());
  TEST(downloader.WaitForRequests(2), ());

  downloader.Complete(0, FailedDownload());
  TEST_EQUAL(ready.Count(), size_t{0}, ("The cancelled request must not consume its replacement"));

  downloader.Complete(1, FailedDownload());
  TEST_EQUAL(ready.Count(key), size_t{1}, ());
}

UNIT_TEST(RasterTileProvider_ConcurrentCompletionsDeliverEachRequestOnce)
{
  Platform::ThreadRunner runner;
  std::string const cacheSubdir = "raster_tile_provider_concurrent_test";
  platform::tests_support::ScopedDirCleanup cleanup(base::JoinPath(GetPlatform().WritableDir(), cacheSubdir));
  ControlledDownloader downloader;
  ReadyRecorder ready;
  RTP provider(TestParams(cacheSubdir), ready.GetReadyFn(), downloader.GetDownloadFn());

  size_t constexpr kRequestCount = 8;
  std::vector<df::TileKey> keys;
  keys.reserve(kRequestCount);
  for (size_t i = 0; i < kRequestCount; ++i)
  {
    keys.emplace_back(static_cast<int>(i) - 4, 0, 6);
    TEST(provider.RequestTile(keys.back(), dp::BackgroundMode::Satellite), (i));
  }
  TEST(downloader.WaitForRequests(kRequestCount), ());

  std::mutex gateMutex;
  std::condition_variable gateCv;
  bool start = false;
  std::vector<std::thread> threads;
  threads.reserve(kRequestCount);
  for (size_t i = 0; i < kRequestCount; ++i)
  {
    threads.emplace_back([&, i]
    {
      {
        std::unique_lock lock(gateMutex);
        gateCv.wait(lock, [&start] { return start; });
      }
      downloader.Complete(i, FailedDownload());
    });
  }
  {
    std::lock_guard lock(gateMutex);
    start = true;
  }
  gateCv.notify_all();
  for (auto & thread : threads)
    thread.join();

  TEST_EQUAL(ready.Count(), kRequestCount, ());
  for (auto const & key : keys)
    TEST_EQUAL(ready.Count(key), size_t{1}, (key));
}
