#include "testing/testing.hpp"

#include "storage/background_downloading/downloader_queue.hpp"

#include "platform/mwm_version.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

namespace background_downloader_tests
{
using namespace storage;

CountryId const kWest = "Wonderland_West";
CountryId const kEast = "Wonderland_East";

class BackgroundDownloaderQueueTest : public QueuedCountry::Subscriber
{
public:
  void OnCountryInQueue(QueuedCountry const & country) override
  {
    if (m_onQueue)
      m_onQueue(country);
  }
  void OnStartDownloading(QueuedCountry const &) override {}
  void OnDownloadProgress(QueuedCountry const &, downloader::Progress const &) override {}
  void OnDownloadFinished(QueuedCountry const &, downloader::DownloadStatus) override {}

protected:
  std::weak_ptr<int> Enqueue(CountryId const & id)
  {
    QueuedCountry country(platform::CountryFile(id, 1, {}), id, MapFileType::Map, version::FOR_TESTING_MWM1, {},
                          std::make_shared<diffs::DiffsDataSource>());
    country.Subscribe(*this);
    return m_queue.Append(std::move(country));
  }

  BackgroundDownloaderQueue<uint64_t> m_queue;
  std::function<void(QueuedCountry const &)> m_onQueue;
};

UNIT_CLASS_TEST(BackgroundDownloaderQueueTest, CancelFromQueueCallback)
{
  for (bool const clear : {false, true})
  {
    bool notified = false;
    m_onQueue = [&](QueuedCountry const & country)
    {
      if (clear)
        m_queue.Clear();
      else
        m_queue.Remove(country.GetCountryId());
      TEST_EQUAL(country.GetCountryId(), kWest, (clear));
      notified = true;
    };
    auto const lifetime = Enqueue(kWest);
    TEST(notified, (clear));
    TEST(m_queue.IsEmpty(), (clear));
    TEST(lifetime.expired(), (clear));
  }
}

UNIT_CLASS_TEST(BackgroundDownloaderQueueTest, AttemptLifetime)
{
  auto const first = Enqueue(kWest);
  auto const unrelated = Enqueue(kEast);
  TEST(!first.expired(), ());
  m_queue.Remove(kWest);
  TEST(first.expired(), ());
  auto const replacement = Enqueue(kWest);
  TEST(!replacement.expired(), ());
  TEST(!unrelated.expired(), ());
  m_queue.Clear();
  TEST(replacement.expired(), ());
  TEST(unrelated.expired(), ());
}

UNIT_CLASS_TEST(BackgroundDownloaderQueueTest, ReplaceFromQueueCallback)
{
  bool replaced = false;
  std::weak_ptr<int> replacement;
  m_onQueue = [&](QueuedCountry const & country)
  {
    if (replaced)
      return;
    replaced = true;
    m_queue.Remove(country.GetCountryId());
    replacement = Enqueue(country.GetCountryId());
  };
  auto const first = Enqueue(kWest);
  TEST(first.expired(), ());
  TEST(!replacement.expired(), ());
  TEST_EQUAL(m_queue.Count(), 1, ());
  TEST(m_queue.Contains(kWest), ());
}
}  // namespace background_downloader_tests
