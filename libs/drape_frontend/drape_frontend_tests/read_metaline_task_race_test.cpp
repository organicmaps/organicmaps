#include "testing/testing.hpp"

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/read_metaline_task.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "base/logging.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// Reproduces the Play Console SIGSEGV reported at df::ReadMetalineTask::Run()+34.
//
// Root cause: ReadMetalineTask::Run() reaches mwmId.GetInfo()->GetLocalFile() (via the inlined
// ReadMetalinesFromFile) without holding MwmSet::m_lock. MwmSet::Register() mutates info->m_file
// under m_lock during a same-version re-registration (mwm_set.cpp:131), racing with the lock-free
// read by the worker thread. The torn read on the underlying std::string / uint64_t fields is UB
// — manifests as a SIGSEGV in production, and as a TSAN data race here.
//
//   cmake -B build-tsan -S . -DCMAKE_BUILD_TYPE=Debug -DUSE_TSAN=ON -GNinja
//   cmake --build build-tsan --target drape_frontend_tests
//   env TSAN_OPTIONS=halt_on_error=1 build-tsan/drape_frontend_tests \
//     --filter=ReadMetalineTask_ConcurrentRegisterReproducesPlayConsoleCrash
//
// Without TSAN the test usually completes cleanly: the racy std::string reads inside
// GetLocalFile().GetPath() rarely tear visibly on x86_64 / arm64, and TSAN itself only reliably
// flags simple-typed races on the same heap object — so the reader loop also takes one direct
// uint64 read on the just-Run() MwmInfo to give the sanitizer something it can pin down. Under
// TSAN the report's writer stack is MwmSet::Register at mwm_set.cpp:131 (the same site the
// production crash races with), while the reader executes Run() each iteration immediately
// before the flagged read — proving the production code path operates on the unsynchronized
// MwmInfo.

namespace read_metaline_task_race_test
{
class TestMwmSet : public MwmSet
{
protected:
  std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const &) const override
  {
    auto info = std::make_unique<MwmInfo>();
    info->m_version.SetFormat(version::Format::lastFormat);
    return info;
  }

  std::unique_ptr<MwmValue> CreateValue(MwmInfo & info) const override
  {
    return std::make_unique<MwmValue>(info.GetLocalFile());
  }
};

df::MapDataProvider MakeEmptyProvider()
{
  return df::MapDataProvider([](auto const &, m2::RectD const &, int) {},
                             [](auto const &, std::vector<FeatureID> const &) {}, [](std::string_view) { return true; },
                             [](m2::PointD const &, int) {}, [](df::TileKey const &, dp::BackgroundMode) {},
                             [](df::TileKey const &, dp::BackgroundMode) {});
}

UNIT_TEST(ReadMetalineTask_ConcurrentRegisterReproducesPlayConsoleCrash)
{
  TestMwmSet mwmSet;
  std::string const country = "7";
  int64_t constexpr kVersion = 42;

  auto makeFile = [&country](std::string directory, MwmSize size = 0)
  {
    return platform::LocalCountryFile(std::move(directory), platform::CountryFile(country, size, std::to_string(size)),
                                      kVersion);
  };

  auto const reg = mwmSet.Register(makeFile(std::string(4096, 'a')));
  TEST_EQUAL(reg.second, MwmSet::RegResult::Success, ());
  auto const id = reg.first;

  auto model = MakeEmptyProvider();

  base::ScopedLogLevelChanger const logLevel(base::LERROR);

  std::atomic<bool> start{false};
  std::atomic<bool> stop{false};
  std::atomic<uint64_t> runs{0};

  std::thread reader([&]()
  {
    while (!start.load(std::memory_order_acquire))
      std::this_thread::yield();

    while (!stop.load(std::memory_order_relaxed))
    {
      // Production code path that crashed: Run() reads info->m_file through GetLocalFile() with
      // no lock (via the inlined ReadMetalinesFromFile -> GetPath -> std::string reads).
      df::ReadMetalineTask task(model, id);
      task.Run();
      // The unlocked read on m_mapSize hits the same MwmInfo object that Run() just touched.
      // TSAN reports this 8-byte race reliably (the std::string reads inside Run() are the same
      // hazard but harder for the sanitizer to flag — see Stock vs TSAN notes above).
      auto const remoteSize = id.GetInfo()->GetLocalFile().GetCountryFile().GetRemoteSize();
      (void)remoteSize;
      runs.fetch_add(1, std::memory_order_release);
    }
  });

  std::thread writer([&]()
  {
    start.store(true, std::memory_order_release);
    while (runs.load(std::memory_order_acquire) < 256)
      std::this_thread::yield();

    for (size_t i = 0; i < 50000; ++i)
    {
      // Alternating sizes force std::string reallocation between SSO and the heap, widening the
      // window for a torn read of m_directory's pointer/size. The MwmSize argument moves
      // CountryFile::m_mapSize so TSAN sees the concrete write.
      std::string directory(i % 2 == 0 ? 4096 : 8192, static_cast<char>('a' + i % 26));
      directory += std::to_string(i);
      TEST_EQUAL(mwmSet.Register(makeFile(std::move(directory), i + 1)).second, MwmSet::RegResult::VersionAlreadyExists,
                 ());
      if (i % 16 == 0)
        std::this_thread::yield();
    }
    stop.store(true, std::memory_order_release);
  });

  writer.join();
  reader.join();
  TEST_GREATER(runs.load(std::memory_order_relaxed), 0, ());
}
}  // namespace read_metaline_task_race_test
