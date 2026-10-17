#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/raster_tile_provider.hpp"

#include "platform/gui_thread.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"

#include "coding/blake3.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/hex.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
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
char constexpr kUrlA[] = "https://tiles.test/{z}/{x}/{y}.png";
char constexpr kUrlB[] = "https://other.test/{z}/{x}/{y}.png?key=test-key";

// A red RGBA pixel, encoded as PNG. Both the HTTP and disk-cache paths must decode it.
uint8_t constexpr kPng[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
                            0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
                            0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
                            0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0, 0x1f, 0x00, 0x05, 0x00, 0x01, 0xff, 0x89, 0x99,
                            0x3d, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

uint8_t constexpr kGreenPng[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
                                 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
                                 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
                                 0x9c, 0x63, 0x60, 0xf8, 0xcf, 0xf0, 0x1f, 0x00, 0x04, 0x01, 0x01, 0xff, 0x71, 0xeb,
                                 0x47, 0xe5, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

HttpClient::Result Success(bool green = false)
{
  HttpClient::Result result;
  result.m_success = true;
  result.m_errorCode = 200;
  result.m_serverResponse.assign(reinterpret_cast<char const *>(green ? kGreenPng : kPng), sizeof(kPng));
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

void CompleteOnTransportThread(HttpClient::CompletionHandler handler, bool green = false)
{
  std::thread worker([handler = std::move(handler), green] { handler(Success(green)); });
  worker.join();
}

class TestGuiThread : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    std::lock_guard lock(m_mutex);
    m_tasks.push_back(std::move(task));
    return {true, kNoId};
  }

  PushResult Push(Task const & task) override { return Push(Task(task)); }

  size_t ExecuteTasks()
  {
    std::vector<Task> tasks;
    {
      std::lock_guard lock(m_mutex);
      tasks.swap(m_tasks);
    }
    for (auto & task : tasks)
      task();
    return tasks.size();
  }

private:
  std::mutex m_mutex;
  std::vector<Task> m_tasks;
};

struct Delivery
{
  df::TileKey m_tile;
  std::string m_uid;
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

  RasterTileProvider::Params Params(uint64_t maxCacheBytes = 1024, std::string const & url = kUrlA) const
  {
    RasterTileProvider::Params params;
    params.m_urlTemplate = url;
    params.m_cacheSubdir = m_subdir;
    params.m_maxCacheBytes = maxCacheBytes;
    return params;
  }

  RasterTileProvider::TReadyFn Ready()
  {
    return [this](df::TileKey const & tile, dp::BackgroundMode, std::string const & uid, uint32_t w, uint32_t h,
                  m2::RectF const &, std::vector<uint8_t> && rgba)
    { m_deliveries.push_back({tile, uid, w, h, std::move(rgba), std::this_thread::get_id()}); };
  }

  RasterTileProvider::TDownloadFn Download()
  {
    // Only the serial File queue appends; tests read after DrainFile synchronizes with it.
    return [this](std::string const & url, HttpClient::CompletionHandler handler)
    {
      m_urls.push_back(url);
      m_pending.push_back(std::move(handler));
    };
  }

  std::unique_ptr<RasterTileProvider> MakeProvider(uint64_t maxCacheBytes = 1024, std::string const & url = kUrlA)
  {
    return std::make_unique<RasterTileProvider>(Params(maxCacheBytes, url), Ready(), Download());
  }

  std::string CacheDir() const { return base::JoinPath(GetPlatform().WritableDir(), m_subdir); }

  // Fixtures that seed a known entry use the cache filename protocol without inspecting provider state.
  std::string CachePath() const
  {
    return base::JoinPath(CacheDir(), ToHex(coding::Blake3::CalculateForString(kUrlA)) + "_9_257_254.tile");
  }

  Platform::FilesList CacheFiles() const
  {
    Platform::FilesList files;
    Platform::GetFilesByRegExp(CacheDir(), ".*\\.tile$", files);
    return files;
  }

  uint64_t CacheBytes() const
  {
    uint64_t total = 0;
    for (auto const & name : CacheFiles())
    {
      uint64_t size = 0;
      TEST(Platform::GetFileSizeByFullPath(base::JoinPath(CacheDir(), name), size), ());
      total += size;
    }
    return total;
  }

  void CheckDelivery(size_t index = 0, bool green = false) const
  {
    TEST_LESS(index, m_deliveries.size(), ());
    auto const & delivered = m_deliveries[index];
    TEST_EQUAL(delivered.m_width, 1, ());
    TEST_EQUAL(delivered.m_height, 1, ());
    TEST_EQUAL(delivered.m_rgba, (green ? std::vector<uint8_t>{0, 255, 0, 255} : std::vector<uint8_t>{255, 0, 0, 255}),
               ());
    TEST(delivered.m_thread == std::this_thread::get_id(), ());
  }

  std::vector<Delivery> m_deliveries;
  std::vector<HttpClient::CompletionHandler> m_pending;
  std::vector<std::string> m_urls;

private:
  std::string const m_subdir;
  platform::tests_support::ScopedDirCleanup m_cleanup;
};

void CheckInstalledGuiThreadDelivery(bool cacheHit)
{
  Fixture f(cacheHit ? "custom_gui_cache" : "custom_gui_http");
  Platform::ThreadRunner runner;
  auto guiThread = std::make_unique<TestGuiThread>();
  auto & queue = *guiThread;
  GetPlatform().SetGuiThread(std::move(guiThread));
  SCOPE_GUARD(restoreGui, [] { GetPlatform().SetGuiThread(std::make_unique<platform::GuiThread>()); });
  if (cacheHit)
  {
    TEST(Platform::MkDirRecursively(f.CacheDir()), ());
    FileWriter writer(f.CachePath());
    writer.Write(kPng, sizeof(kPng));
  }
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  if (cacheHit)
  {
    TEST(f.m_pending.empty(), ("A disk-cache hit must not start an HTTP request"));
  }
  else
  {
    TEST_EQUAL(f.m_pending.size(), 1, ());
    CompleteOnTransportThread(std::move(f.m_pending[0]));
  }
  TEST(f.m_deliveries.empty(), ());
  auto const executed = queue.ExecuteTasks();
  TEST_EQUAL(executed, 1, ("The installed GUI queue must receive the delivery"));
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  f.CheckDelivery();
}

class TestFramework : public Framework
{
public:
  TestFramework() : Framework({}, false /* loadMaps */) {}

  void SetProvider(std::unique_ptr<RasterTileProvider> provider) { m_rasterTileProvider = std::move(provider); }
};
}  // namespace

UNIT_TEST(RasterTileProvider_HttpDeliveryUsesInstalledGuiThread)
{
  CheckInstalledGuiThreadDelivery(false /* cacheHit */);
}

UNIT_TEST(RasterTileProvider_CacheDeliveryUsesInstalledGuiThread)
{
  CheckInstalledGuiThreadDelivery(true /* cacheHit */);
}

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

UNIT_TEST(RasterTileProvider_SourceChangeRejectsOldHttpAndCacheWrite)
{
  for (bool const cancelled : {false, true})
  {
    Fixture f(cancelled ? "cancelled_source_http" : "pending_source_http");
    Platform::ThreadRunner runner;
    auto provider = f.MakeProvider();
    TEST(provider->RequestTile(kTile, kMode), ());
    DrainFile();
    TEST_EQUAL(f.m_pending.size(), 1, ());
    if (cancelled)
      provider->CancelTile(kTile, kMode);
    TEST(provider->Reconfigure(kUrlB, 1024), ());
    TEST(provider->RequestTile(kTile, kMode), ());
    DrainFile();
    TEST_EQUAL(f.m_pending.size(), 2, ());
    TEST_EQUAL(f.m_urls[1], "https://other.test/9/257/254.png?key=test-key", ());

    CompleteOnTransportThread(std::move(f.m_pending[0]));
    DrainGui();
    TEST(f.m_deliveries.empty(), ());
    TEST(f.CacheFiles().empty(), ("The obsolete success must not populate the disk cache"));
    CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
    DrainGui();
    TEST_EQUAL(f.m_deliveries.size(), 1, ());
    f.CheckDelivery(0, true /* green */);
    TEST(f.m_deliveries[0].m_uid.find("test-key") == std::string::npos, ());

    provider.reset();
    provider = f.MakeProvider();
    TEST(provider->RequestTile(kTile, kMode), ());
    DrainFile();
    TEST_EQUAL(f.m_pending.size(), 3, ("Returning to A must not find the rejected old response"));
  }
}

UNIT_TEST(RasterTileProvider_SourceChangeRejectsQueuedGuiDelivery)
{
  Fixture f("source_gui");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  TEST(f.m_deliveries.empty(), ());

  TEST(provider->Reconfigure(kUrlB, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST(f.m_deliveries.empty(), ());
  TEST_EQUAL(f.m_pending.size(), 2, ("B must download its own bytes"));
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  DrainGui();
  f.CheckDelivery(0, true /* green */);

  TEST(provider->Reconfigure(kUrlA, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 2, ());
  TEST_EQUAL(f.m_pending.size(), 2, ("A was cached before the switch and remains reusable"));
  f.CheckDelivery(1);
  TEST_NOT_EQUAL(f.m_deliveries[0].m_uid, f.m_deliveries[1].m_uid, ());
}

UNIT_TEST(RasterTileProvider_ReconstructionUsesStableSourceIdentity)
{
  Fixture f("reconstructed_source");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  f.CheckDelivery();
  auto const uidA = f.m_deliveries[0].m_uid;

  provider.reset();
  provider = f.MakeProvider(1024, kUrlB);
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 1, ("A's persistent cache must not satisfy a new B provider"));
  TEST_EQUAL(f.m_pending.size(), 2, ());
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  DrainGui();
  f.CheckDelivery(1, true /* green */);
  TEST_NOT_EQUAL(f.m_deliveries[1].m_uid, uidA, ());

  provider.reset();
  provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_pending.size(), 2, ("The same source reuses its cache across reconstruction"));
  TEST_EQUAL(f.m_deliveries.size(), 3, ());
  TEST_EQUAL(f.m_deliveries[2].m_uid, uidA, ());
  f.CheckDelivery(2);
}

UNIT_TEST(RasterTileProvider_RapidSourceChangesRejectEarlierRequests)
{
  Fixture f("source_roundtrip");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST(provider->Reconfigure(kUrlB, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST(provider->Reconfigure(kUrlA, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 3, ());

  CompleteOnTransportThread(std::move(f.m_pending[2]));
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  CompleteOnTransportThread(std::move(f.m_pending[0]), true /* green */);
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  f.CheckDelivery();

  provider.reset();
  provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_pending.size(), 3, ());
  TEST_EQUAL(f.m_deliveries.size(), 2, ());
  f.CheckDelivery(1);
}

UNIT_TEST(RasterTileProvider_SourceChangeRejectsQueuedFileRead)
{
  Fixture f("source_file");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();

  auto release = std::make_shared<std::promise<void>>();
  auto released = release->get_future().share();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [released] { released.wait(); }).m_isSuccess, ());
  SCOPE_GUARD(unblock, [release] { release->set_value(); });
  TEST(provider->RequestTile(kTile, kMode), ());
  TEST(provider->Reconfigure(kUrlB, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  release->set_value();
  unblock.release();
  DrainFile();
  DrainGui();

  TEST_EQUAL(f.m_deliveries.size(), 1, ("The queued A read must not deliver its cached pixel"));
  TEST_EQUAL(f.m_pending.size(), 2, ());
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 2, ());
  f.CheckDelivery(1, true /* green */);
}

UNIT_TEST(RasterTileProvider_SameUrlAndCapChangesKeepRequestsAndCache)
{
  Fixture f("same_source");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  TEST(!provider->Reconfigure(kUrlA, sizeof(kPng)), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 1, ());

  TEST(!provider->Reconfigure(kUrlA, 1024), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  TEST_EQUAL(f.m_deliveries.size(), 2, ());
  TEST_EQUAL(f.m_deliveries[0].m_uid, f.m_deliveries[1].m_uid, ());
  f.CheckDelivery(1);

  TEST(!provider->Reconfigure(kUrlA, 0), ());
  TEST(f.CacheFiles().empty(), ("A cap-only change must still evict disk files"));
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 2, ());
}

UNIT_TEST(RasterTileProvider_FailedCacheCommitDoesNotIndexOrLeak)
{
  Fixture f("failed_cache_commit");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ());
  TEST_EQUAL(Platform::MkDir(f.CachePath()), Platform::ERR_OK, ());

  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  f.CheckDelivery();

  {
    // Eviction must not try to remove a destination that was never cached.
    base::ScopedLogAbortLevelChanger const noWarnings(LWARNING);
    TEST(!provider->Reconfigure(kUrlA, 0), ());
  }
  TEST(Platform::IsDirectory(f.CachePath()), ("A failed cache commit must not index its destination"));
  Platform::FilesList temporaryFiles;
  Platform::GetFilesByRegExp(f.CacheDir(), ".*\\.tmp.*", temporaryFiles);
  TEST(temporaryFiles.empty(), (temporaryFiles));
}

UNIT_TEST(RasterTileProvider_GlobalCapIncludesLegacyAndOtherSources)
{
  Fixture f("global_source_cap");
  Platform::ThreadRunner runner;
  TEST(Platform::MkDirRecursively(f.CacheDir()), ());
  for (auto const & name : {"9_257_254.tile", "another_source.tile"})
  {
    FileWriter writer(base::JoinPath(f.CacheDir(), name));
    writer.Write(kPng, sizeof(kPng));
  }
  auto provider = f.MakeProvider(3 * sizeof(kPng));
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 1, ("Legacy coordinates cannot identify A's source"));
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  TEST_EQUAL(f.CacheBytes(), 3 * sizeof(kPng), ());

  TEST(provider->Reconfigure(kUrlB, 4 * sizeof(kPng)), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  DrainGui();
  TEST_EQUAL(f.CacheBytes(), 4 * sizeof(kPng), ());
  TEST(!provider->Reconfigure(kUrlB, sizeof(kPng)), ());
  TEST_EQUAL(f.CacheBytes(), sizeof(kPng), ());
  TEST_EQUAL(f.CacheFiles().size(), 1, ());

  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  DrainGui();
  TEST_EQUAL(f.m_pending.size(), 2, ("B is the most recent entry and must survive the smaller cap"));
  f.CheckDelivery(2, true /* green */);
  TEST(provider->Reconfigure(kUrlA, sizeof(kPng)), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  TEST_EQUAL(f.m_pending.size(), 3, ("The same global cap must have evicted A"));
}

UNIT_TEST(RasterTileProvider_SourceChangeBeforeInitialScanKeepsGlobalCap)
{
  Fixture f("source_before_scan");
  Platform::ThreadRunner runner;
  auto provider = f.MakeProvider();
  TEST(provider->RequestTile(kTile, kMode), ());
  DrainFile();
  CompleteOnTransportThread(std::move(f.m_pending[0]));
  DrainGui();
  provider.reset();
  {
    FileWriter writer(base::JoinPath(f.CacheDir(), "9_257_254.tile"));
    writer.Write(kPng, sizeof(kPng));
  }

  auto release = std::make_shared<std::promise<void>>();
  auto released = release->get_future().share();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [released] { released.wait(); }).m_isSuccess, ());
  SCOPE_GUARD(unblock, [release] { release->set_value(); });
  provider = f.MakeProvider();
  TEST(provider->Reconfigure(kUrlB, sizeof(kPng)), ());
  TEST(provider->RequestTile(kTile, kMode), ());
  release->set_value();
  unblock.release();
  DrainFile();
  DrainGui();

  TEST_EQUAL(f.CacheBytes(), sizeof(kPng), ("The delayed scan must include legacy and A entries"));
  TEST_EQUAL(f.m_deliveries.size(), 1, ());
  TEST_EQUAL(f.m_pending.size(), 2, ());
  CompleteOnTransportThread(std::move(f.m_pending[1]), true /* green */);
  DrainGui();
  TEST_EQUAL(f.CacheFiles().size(), 1, ("No pre-scan entry may escape the shared LRU cap"));
  TEST_EQUAL(f.CacheBytes(), sizeof(kPng), ());
  f.CheckDelivery(1, true /* green */);
}
}  // namespace raster_tile_provider_tests
