#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/raster_tile_provider.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <QCoreApplication>
#include <QPointer>
#include <QTimer>

namespace raster_tile_provider_tests
{
namespace
{
using namespace std::chrono_literals;
using HttpClient = platform::HttpClient;
auto constexpr kMode = dp::BackgroundMode::Satellite;
df::TileKey const kTile(1, 1, 10);

// A red RGBA pixel, encoded as PNG. Both the HTTP and disk-cache paths must decode it.
uint8_t constexpr kPng[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
                            0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
                            0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
                            0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0, 0x1f, 0x00, 0x05, 0x00, 0x01, 0xff, 0x89, 0x99,
                            0x3d, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

HttpClient::Result Success()
{
  HttpClient::Result result;
  result.m_success = true;
  result.m_errorCode = 200;
  result.m_serverResponse.assign(reinterpret_cast<char const *>(kPng), sizeof(kPng));
  return result;
}

void DrainFile()
{
  auto done = std::make_shared<std::promise<void>>();
  auto finished = done->get_future();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [done] { done->set_value(); }).m_isSuccess, ());
  TEST(finished.wait_for(5s) == std::future_status::ready, ());
}

void DrainGui()
{
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, QCoreApplication::instance(), &QCoreApplication::quit);
  auto drained = std::make_shared<bool>(false);
  QPointer<QTimer> guard(&timeout);
  // Posting off the GUI thread also tests Qt, whose GUI-thread Push may execute inline.
  std::thread poster([drained, guard]
  {
    platform::GuiThread().Push([drained, guard]
    {
      *drained = true;
      if (guard)
        QCoreApplication::quit();
    });
  });
  poster.join();
  timeout.start(5000);
  // The application loop also services Apple's native main queue.
  QCoreApplication::exec();
  TEST(*drained, ());
}

void CompleteOnTransportThread(HttpClient::CompletionHandler handler)
{
  std::thread worker([handler = std::move(handler)] { handler(Success()); });
  worker.join();
}

struct Delivery
{
  df::TileKey m_tile;
  uint32_t m_width;
  uint32_t m_height;
  std::vector<uint8_t> m_rgba;
  std::thread::id m_thread;
};

class Fixture
{
public:
  explicit Fixture(std::string const & name)
    : m_subdir("raster_lifetime_" + name)
    , m_cleanup(base::JoinPath(GetPlatform().WritableDir(), m_subdir))
  {}

  RasterTileProvider::Params Params(uint64_t maxCacheBytes = 1024) const
  {
    RasterTileProvider::Params params;
    params.m_urlTemplate = "https://tiles.test/{z}/{x}/{y}.png";
    params.m_cacheSubdir = m_subdir;
    params.m_maxCacheBytes = maxCacheBytes;
    return params;
  }

  RasterTileProvider::TReadyFn Ready()
  {
    return [this](df::TileKey const & tile, dp::BackgroundMode, std::string const &, uint32_t w, uint32_t h,
                  m2::RectF const &, std::vector<uint8_t> && rgba)
    { m_deliveries.push_back({tile, w, h, std::move(rgba), std::this_thread::get_id()}); };
  }

  RasterTileProvider::TDownloadFn Download()
  {
    // Only the serial File queue appends; tests read after DrainFile synchronizes with it.
    return [this](std::string const &, HttpClient::CompletionHandler handler)
    { m_pending.push_back(std::move(handler)); };
  }

  std::unique_ptr<RasterTileProvider> MakeProvider(uint64_t maxCacheBytes = 1024)
  {
    return std::make_unique<RasterTileProvider>(Params(maxCacheBytes), Ready(), Download());
  }

  std::string CachePath() const { return base::JoinPath(GetPlatform().WritableDir(), m_subdir, "9_257_254.tile"); }

  void CheckDelivery(size_t index = 0) const
  {
    TEST_LESS(index, m_deliveries.size(), ());
    auto const & delivered = m_deliveries[index];
    TEST_EQUAL(delivered.m_width, 1, ());
    TEST_EQUAL(delivered.m_height, 1, ());
    TEST_EQUAL(delivered.m_rgba, (std::vector<uint8_t>{255, 0, 0, 255}), ());
    TEST(delivered.m_thread == std::this_thread::get_id(), ());
  }

  std::vector<Delivery> m_deliveries;
  std::vector<HttpClient::CompletionHandler> m_pending;

private:
  std::string const m_subdir;
  platform::tests_support::ScopedDirCleanup m_cleanup;
};

class TestFramework : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}

  void SetProvider(std::unique_ptr<RasterTileProvider> provider) { m_rasterTileProvider = std::move(provider); }
};
}  // namespace

UNIT_TEST(RasterTileProvider_HeldHttpSuccessAfterDestruction)
{
  Fixture f("late_http");
  auto runner = std::make_unique<Platform::ThreadRunner>();
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());

  provider.reset();
  runner.reset();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  TEST(f.m_deliveries.empty(), ());
  TEST(!GetPlatform().IsFileExistsByFullPath(f.CachePath()), ());
}

UNIT_TEST(RasterTileProvider_StoppedHttpCannotOverwriteReplacementCache)
{
  Fixture f("replacement_cache");
  Platform::ThreadRunner runner;
  auto oldProvider = f.MakeProvider(0 /* maxCacheBytes */);
  TEST(oldProvider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  oldProvider.reset();

  auto replacement = f.MakeProvider();
  TEST(replacement->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 2, ());
  auto newImage = Success();
  // Extra encoded bytes distinguish the new cache entry while preserving the decoded pixel.
  newImage.m_serverResponse += "replacement";
  f.m_pending[1](newImage);
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  f.CheckDelivery();

  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  std::string cached;
  FileReader(f.CachePath()).ReadAsString(cached);
  TEST_EQUAL(cached, newImage.m_serverResponse, ());
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
}

UNIT_TEST(RasterTileProvider_QueuedFileWorkAfterDestruction)
{
  Fixture f("queued_file");
  Platform::ThreadRunner runner;
  auto release = std::make_shared<std::promise<void>>();
  auto released = release->get_future().share();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [released] { released.wait(); }).m_isSuccess, ());
  SCOPE_GUARD(unblock, [release] { release->set_value(); });

  auto oldProvider = f.MakeProvider(0 /* maxCacheBytes */);
  TEST(oldProvider->RequestTile(kTile, kMode), ());
  oldProvider.reset();
  auto replacement = f.MakeProvider();
  {
    FileWriter writer(f.CachePath());
    writer.Write(kPng, sizeof(kPng));
  }
  TEST(replacement->RequestTile(kTile, kMode), ());
  release->set_value();
  unblock.release();
  DrainFile();
  DrainGui();

  TEST(f.m_pending.empty(), ());
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  f.CheckDelivery();
  TEST(GetPlatform().IsFileExistsByFullPath(f.CachePath()), ());
}

UNIT_TEST(RasterTileProvider_RunningFileWorkRetainsState)
{
  Fixture f("running_file");
  Platform::ThreadRunner runner;
  auto entered = std::make_shared<std::promise<void>>();
  auto started = entered->get_future();
  auto release = std::make_shared<std::promise<void>>();
  auto released = release->get_future().share();
  SCOPE_GUARD(unblock, [release] { release->set_value(); });
  auto provider =
      std::make_unique<RasterTileProvider>(f.Params(), f.Ready(), [entered, released](auto const &, auto handler)
  {
    entered->set_value();
    released.wait();
    // StartDownload is still on the File stack when the wrapper is destroyed.
    handler(Success());
  });
  TEST(provider->RequestTile(kTile, kMode), ());
  TEST(started.wait_for(5s) == std::future_status::ready, ());
  provider.reset();
  release->set_value();
  unblock.release();
  DrainFile();
  DrainGui();
  TEST(f.m_deliveries.empty(), ());
  TEST(!GetPlatform().IsFileExistsByFullPath(f.CachePath()), ());
}

UNIT_TEST(RasterTileProvider_QueuedGuiDeliveryDroppedAfterShutdown)
{
  Fixture f("queued_gui");
  auto runner = std::make_unique<Platform::ThreadRunner>();
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  TEST(f.m_deliveries.empty(), ());

  provider->Shutdown();
  TEST(!provider->RequestTile(kTile, kMode), ("Stopped providers reject work even while worker queues are running"));
  runner.reset();
  // Render callbacks retain the wrapper until engine teardown and must reject new submissions.
  bool accepted = true;
  std::thread render([&]
  {
    accepted = provider->RequestTile(kTile, kMode);
    provider->CancelTile(kTile, kMode);
  });
  render.join();
  TEST(!accepted, ());
  provider.reset();
  DrainGui();
  TEST(f.m_deliveries.empty(), ());
}

UNIT_TEST(RasterTileProvider_GuiAffinityAndReentrantDestruction)
{
  for (bool const completeOnGui : {false, true})
  {
    Fixture f(completeOnGui ? "gui_completion" : "transport_completion");
    Platform::ThreadRunner runner;
    std::unique_ptr<RasterTileProvider> provider;
    auto ready = f.Ready();
    provider = std::make_unique<RasterTileProvider>(f.Params(), [&](auto &&... args)
    {
      ready(std::forward<decltype(args)>(args)...);
      provider.reset();
    }, f.Download());
    TEST(provider->RequestTile(kTile, kMode), ());
    DrainFile();
    TEST_EQUAL(f.m_pending.size(), 1, ());
    if (completeOnGui)
      f.m_pending[0](Success());
    else
      CompleteOnTransportThread(std::move(f.m_pending[0]));
    DrainGui();
    TEST(!provider, ());
    TEST_EQUAL(f.m_deliveries.size(), 1, ());
    f.CheckDelivery();
  }
}

UNIT_TEST(RasterTileProvider_CancelRerequestRejectsOldHttpCompletion)
{
  for (uint64_t const generation : {0, 42})
  {
    Fixture f(generation == 0 ? "cancel_http_same_generation" : "cancel_http_new_generation");
    Platform::ThreadRunner runner;
    RasterTileProvider provider(f.Params(), f.Ready(), f.Download());
    TEST(provider.RequestTile(kTile, kMode), ());
    DrainFile();
    provider.CancelTile(kTile, kMode);
    df::TileKey const newTile(kTile, generation, 0);
    TEST(provider.RequestTile(newTile, kMode), ());
    DrainFile();
    TEST_EQUAL(f.m_pending.size(), 2, ());

    CompleteOnTransportThread(std::move(f.m_pending[0]));
    DrainGui();
    TEST(f.m_deliveries.empty(), ());
    CompleteOnTransportThread(std::move(f.m_pending[1]));
    DrainGui();
    TEST_EQUAL(f.m_deliveries.size(), 1, ());
    TEST_EQUAL(f.m_deliveries[0].m_tile.m_generation, generation, ());
  }
}

UNIT_TEST(RasterTileProvider_CancelRerequestRejectsQueuedGuiDelivery)
{
  Fixture f("cancel_gui");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  provider->CancelTile(kTile, kMode);
  df::TileKey const newTile(kTile, 42, 0);
  TEST(provider->RequestTile(newTile, kMode), ());
  DrainFile();
  DrainGui();

  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  TEST_EQUAL(f.m_deliveries[0].m_tile.m_generation, 42, ());
  TEST_EQUAL(f.m_pending.size(), 1, ("The new request should hit the cache"));
  f.CheckDelivery();
}

UNIT_TEST(RasterTileProvider_FrameworkStopsProviderBeforeWorkers)
{
  Fixture f("framework_shutdown");
  auto framework = std::make_unique<TestFramework>();
  auto provider = f.MakeProvider();
  auto * rawProvider = provider.get();
  framework->SetProvider(std::move(provider));
  TEST(rawProvider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  framework.reset();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  TEST(f.m_deliveries.empty(), ());
  TEST(!GetPlatform().IsFileExistsByFullPath(f.CachePath()), ());
}

UNIT_TEST(RasterTileProvider_FrameworkRecreationDropsOldGuiDelivery)
{
  Fixture f("framework_recreation");
  TestFramework framework;
  auto oldProvider = f.MakeProvider();
  TEST(oldProvider->RequestTile(kTile, kMode), ());
  framework.SetProvider(std::move(oldProvider));
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  CompleteOnTransportThread(std::move(f.m_pending[0]));

  framework.DestroyDrapeEngine();
  auto replacement = f.MakeProvider();
  TEST(replacement->RequestTile(df::TileKey(kTile, 42, 0), kMode), ());
  framework.SetProvider(std::move(replacement));
  DrainFile();
  DrainGui();

  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  TEST_EQUAL(f.m_deliveries[0].m_tile.m_generation, 42, ());
  f.CheckDelivery();
}
}  // namespace raster_tile_provider_tests
