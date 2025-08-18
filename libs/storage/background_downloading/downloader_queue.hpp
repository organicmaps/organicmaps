#pragma once

#include "storage/downloader_queue_interface.hpp"
#include "storage/queued_country.hpp"
#include "storage/storage_defines.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace storage
{
template <typename TaskInfo>
class BackgroundDownloaderQueue : public QueueInterface
{
public:
  using ForEachTaskInfoTypeFunction = std::function<void(TaskInfo const & info)>;

  bool IsEmpty() const override { return m_queue.empty(); }

  size_t Count() const override { return m_queue.size(); }

  bool Contains(CountryId const & country) const override { return m_queue.find(country) != m_queue.cend(); }

  void ForEachCountry(ForEachCountryFunction const & fn) const override
  {
    for (auto const & item : m_queue)
      fn(item.second.m_queuedCountry);
  }

  void ForEachTaskInfo(ForEachTaskInfoTypeFunction const & fn) const
  {
    for (auto const & item : m_queue)
      if (item.second.m_taskInfo)
        fn(*item.second.m_taskInfo);
  }

  void Append(QueuedCountry && country)
  {
    auto const countryId = country.GetCountryId();
    auto const result = m_queue.emplace(countryId, std::move(country));
    result.first->second.m_queuedCountry.OnCountryInQueue();
  }

  void SetTaskInfoForCountryId(CountryId const & countryId, TaskInfo const & taskInfo)
  {
    auto const it = m_queue.find(countryId);
    CHECK(it != m_queue.cend(), ());

    it->second.m_taskInfo = taskInfo;
  }

  std::optional<TaskInfo> GetTaskInfoForCountryId(CountryId const & countryId) const
  {
    auto const it = m_queue.find(countryId);
    if (it == m_queue.cend())
      return {};

    return it->second.m_taskInfo;
  }

  QueuedCountry & GetCountryById(CountryId const & countryId) { return m_queue.at(countryId).m_queuedCountry; }

  void Remove(CountryId const & countryId) { m_queue.erase(countryId); }

  void Clear() { m_queue.clear(); }

private:
  struct TaskData
  {
    explicit TaskData(QueuedCountry && country) : m_queuedCountry(std::move(country)) {}

    QueuedCountry m_queuedCountry;
    std::optional<TaskInfo> m_taskInfo;
  };

  std::unordered_map<CountryId, TaskData> m_queue;
};
}  // namespace storage
