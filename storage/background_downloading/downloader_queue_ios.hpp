#pragma once

#include "storage/downloader_queue.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>

#include <boost/optional.hpp>

namespace storage
{
class BackgroundDownloaderQueue : public QueueInterface
{
public:
  bool IsEmpty() const override;
  bool Contains(CountryId const & country) const override;
  void ForEachCountry(ForEachCountryFunction const & fn) const override;

  void Append(QueuedCountry && country)
  {
    auto const countryId = country.GetCountryId();
    auto const result = m_queue.emplace(countryId, std::move(country));
    result.first->second.m_queuedCountry.OnCountryInQueue();
  }

  void SetTaskIdForCountryId(CountryId const & countryId, uint64_t taskId);
  boost::optional<uint64_t> GetTaskIdByCountryId(CountryId const & countryId) const;

  QueuedCountry & GetCountryById(CountryId const & countryId);

  void Remove(CountryId const & country);
  void Clear();

private:
  struct TaskData
  {
    explicit TaskData(QueuedCountry && country) : m_queuedCountry(std::move(country)) {}

    QueuedCountry m_queuedCountry;
    boost::optional<uint64_t> m_taskId;
  };

  std::unordered_map<CountryId, TaskData> m_queue;
};
}  // namespace storage
