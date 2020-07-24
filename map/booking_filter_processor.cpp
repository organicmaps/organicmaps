#include "map/booking_filter_processor.hpp"
#include "map/booking_availability_filter.hpp"

#include "search/result.hpp"

#include <memory>
#include <utility>

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
  ApplyFiltersInternal(results, std::move(tasks), mode);
}

void FilterProcessor::ApplyFilters(std::vector<FeatureID> && featureIds, TasksRawInternal && tasks,
                                   ApplicationMode const mode)
{
  ApplyFiltersInternal(std::move(featureIds), std::move(tasks), mode);
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
      std::vector<Extras> extras;
      std::vector<FeatureID> filteredOut;
      m_filters.at(type)->GetFeaturesFromCache(results, featuresSorted, extras, filteredOut);

      cachedResults.emplace_back(type, std::move(featuresSorted), std::move(extras),
                                 std::move(filteredOut));
    }

    callback(std::move(cachedResults));
  });
}

DataSource const & FilterProcessor::GetDataSource() const { return m_dataSource; }

Api const & FilterProcessor::GetApi() const
{
  return m_api;
}

template <typename Source, typename TaskInternalType>
void FilterProcessor::ApplyFiltersInternal(Source && source, TaskInternalType && tasks,
                                           ApplicationMode const mode)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, source = std::forward<Source>(source),
                                                 tasks = std::forward<TaskInternalType>(tasks),
                                                 mode]() mutable
  {
    CHECK(!tasks.empty(), ());

    switch (mode)
    {
    case Independent: ApplyIndependently(source, tasks); break;
    case Consecutive: ApplyConsecutively(source, tasks); break;
    }
  });
}

template <typename Source, typename TaskInternalType>
void FilterProcessor::ApplyConsecutively(Source const & source, TaskInternalType & tasks)
{
  for (size_t i = tasks.size() - 1; i > 0; --i)
  {
    auto const & cb = tasks[i - 1].m_filterParams.m_callback;

    tasks[i - 1].m_filterParams.m_callback =
        [ this, cb, nextTask = std::move(tasks[i]) ](auto && result) mutable
    {
    
      auto copy = result;
      cb(std::move(copy));
      // Run the next filter with obtained results from the previous one.
      // Post different task on the file thread to increase granularity.
      // Note: FilterProcessor works on file thread, so all filters will
      // be applied on the single thread.
      GetPlatform().RunTask(
          Platform::Thread::File, [this, passed = std::move(result.m_passedFilter),
                                         nextTask = std::move(nextTask)]() {
            m_filters.at(nextTask.m_type)->ApplyFilter(passed, nextTask.m_filterParams);
          });
      };
  }
  // Run the first filter.
  m_filters.at(tasks.front().m_type)->ApplyFilter(source, tasks.front().m_filterParams);
}

template <typename Source, typename TaskInternalType>
void FilterProcessor::ApplyIndependently(Source const & source, TaskInternalType const & tasks)
{
  for (auto const & task : tasks)
    m_filters.at(task.m_type)->ApplyFilter(source, task.m_filterParams);
}
}  // namespace filter
}  // namespace booking
