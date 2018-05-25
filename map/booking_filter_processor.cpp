#include "map/booking_filter_processor.hpp"
#include "map/booking_availability_filter.hpp"

#include "search/result.hpp"

namespace booking
{
namespace filter
{
FilterProcessor::FilterProcessor(Index const & index, booking::Api const & api)
  : m_index(index)
  , m_api(api)
{
  m_filters.emplace(Type::Deals, std::make_unique<AvailabilityFilter>(*this));
  m_filters.emplace(Type::Availability, std::make_unique<AvailabilityFilter>(*this));
}

void FilterProcessor::ApplyFilters(search::Results const & results,
                                   std::vector<FilterTask> && tasks)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, results, tasks = std::move(tasks)]() mutable
  {
    CHECK(!tasks.empty(), ());

    // Run provided filters consecutively.
    for (size_t i = tasks.size() - 1; i > 0; --i)
    {
      auto const & cb = tasks[i - 1].m_filterParams.m_callback;

      tasks[i - 1].m_filterParams.m_callback =
          [ this, cb, nextTask = std::move(tasks[i]) ](search::Results const & results) mutable
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
    // Run first filter.
    m_filters.at(tasks.front().m_type)->ApplyFilter(results, tasks.front().m_filterParams);
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

void FilterProcessor::GetFeaturesFromCache(Type const type, search::Results const & results,
                                           FillSearchMarksCallback const & callback)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, type, results, callback]()
  {
    std::vector<FeatureID> featuresSorted;
    m_filters.at(type)->GetFeaturesFromCache(results, featuresSorted);

    ASSERT(std::is_sorted(featuresSorted.begin(), featuresSorted.end()), ());
    callback(featuresSorted);
  });
}

Index const & FilterProcessor::GetIndex() const
{
  return m_index;
}

Api const & FilterProcessor::GetApi() const
{
  return m_api;
}
}  // namespace filter
}  // namespace booking
