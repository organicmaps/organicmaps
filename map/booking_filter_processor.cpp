#include "map/booking_filter_processor.hpp"
#include "map/booking_availability_filter.hpp"

#include "search/result.hpp"

namespace booking
{
namespace filter
{
FilterProcessor::FilterProcessor(DataSource const & dataSource, booking::Api const & api)
  : m_dataSource(dataSource), m_api(api)
{
  m_filters.emplace(Type::Deals, std::make_unique<AvailabilityFilter>(*this));
  m_filters.emplace(Type::Availability, std::make_unique<AvailabilityFilter>(*this));
}

void FilterProcessor::ApplyFilters(search::Results const & results, TasksInternal && tasks,
                                   ApplicationMode const mode)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, results, tasks = std::move(tasks), mode]() mutable
  {
    CHECK(!tasks.empty(), ());

    switch (mode)
    {
    case Independent: ApplyIndependently(results, tasks); break;
    case Consecutive: ApplyConsecutively(results, tasks); break;
    }
  });
}

void FilterProcessor::OnParamsUpdated(Type const type,
                                      std::shared_ptr<ParamsBase> const & apiParams)
{
  GetPlatform().RunTask(Platform::Thread::File,
                        [this, type, apiParams = std::move(apiParams)]() mutable
                        {
                          m_filters.at(type)->UpdateParams(*apiParams);
                        });
}

void FilterProcessor::GetFeaturesFromCache(Types const & types, search::Results const & results,
                                           FillSearchMarksCallback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, types, results, callback]()
  {
    CachedResults cachedResults;
    for (auto const type : types)
    {
      std::vector<FeatureID> featuresSorted;
      m_filters.at(type)->GetFeaturesFromCache(results, featuresSorted);

      ASSERT(std::is_sorted(featuresSorted.begin(), featuresSorted.end()), ());

      cachedResults.emplace_back(type, std::move(featuresSorted));
    }

    callback(std::move(cachedResults));
  });
}

DataSource const & FilterProcessor::GetDataSource() const { return m_dataSource; }

Api const & FilterProcessor::GetApi() const
{
  return m_api;
}

void FilterProcessor::ApplyConsecutively(search::Results const & results, TasksInternal & tasks)
{
  for (size_t i = tasks.size() - 1; i > 0; --i)
  {
    auto const & cb = tasks[i - 1].m_filterParams.m_callback;

    tasks[i - 1].m_filterParams.m_callback =
      [this, cb, nextTask = std::move(tasks[i])](search::Results const & results) mutable
      {
        cb(results);
        // Run the next filter with obtained results from the previous one.
        // Post different task on the file thread to increase granularity.
        GetPlatform().RunTask(Platform::Thread::File, [this, results, nextTask = std::move(nextTask)]()
        {
          m_filters.at(nextTask.m_type)->ApplyFilter(results, nextTask.m_filterParams);
        });
      };
  }
  // Run the first filter.
  m_filters.at(tasks.front().m_type)->ApplyFilter(results, tasks.front().m_filterParams);
}

void FilterProcessor::ApplyIndependently(search::Results const & results,
                                         TasksInternal const & tasks)
{
  for (auto const & task : tasks)
    m_filters.at(task.m_type)->ApplyFilter(results, task.m_filterParams);
}
}  // namespace filter
}  // namespace booking
