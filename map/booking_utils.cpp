#include "map/booking_utils.hpp"

#include "map/search_mark.hpp"

#include "search/result.hpp"

#include "indexer/feature_decl.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace booking
{
namespace
{
void Sort(search::Results && results, std::vector<Extras> && extras,
          std::vector<FeatureID> & sortedFeatures, std::vector<booking::Extras> & sortedExtras)
{
  if (!extras.empty())
  {
    CHECK_EQUAL(results.GetCount(), extras.size(), ());

    std::vector<std::pair<FeatureID, booking::Extras>> featuresWithExtras;
    featuresWithExtras.reserve(results.GetCount());
    for (size_t i = 0; i < results.GetCount(); ++i)
      featuresWithExtras.emplace_back(std::move(results[i].GetFeatureID()), std::move(extras[i]));

    std::sort(featuresWithExtras.begin(), featuresWithExtras.end(),
              base::LessBy(&std::pair<FeatureID, booking::Extras>::first));

    sortedFeatures.reserve(featuresWithExtras.size());
    sortedExtras.reserve(featuresWithExtras.size());
    for (auto & item : featuresWithExtras)
    {
      sortedFeatures.emplace_back(std::move(item.first));
      sortedExtras.emplace_back(std::move(item.second));
    }
  }
  else
  {
    sortedFeatures.reserve(results.GetCount());
    for (auto const & r : results)
      sortedFeatures.push_back(r.GetFeatureID());

    std::sort(sortedFeatures.begin(), sortedFeatures.end());
  }
}
}  // namespace

filter::TasksInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                        SearchMarks & searchMarks, bool inViewport)
{
  using namespace booking::filter;

  TasksInternal tasksInternal;

  for (auto const & task : filterTasks)
  {
    auto const type = task.m_type;
    auto const & apiParams = task.m_filterParams.m_apiParams;
    auto const & cb = task.m_filterParams.m_callback;

    if (apiParams->IsEmpty())
      continue;

    ParamsInternal paramsInternal
      {
        apiParams,
        [&searchMarks, type, apiParams, cb, inViewport](auto && results, auto && extras)
        {
          if (results.GetCount() == 0)
            return;

          std::vector<FeatureID> sortedFeatures;
          std::vector<booking::Extras> sortedExtras;
          Sort(std::move(results), std::move(extras), sortedFeatures, sortedExtras);

          if (inViewport)
          {
            // TODO(a): add price formatting for array.
            // auto const pricesFormatted = FormatPrices(sortedExtras);

            GetPlatform().RunTask(Platform::Thread::Gui, [&searchMarks, type, sortedFeatures]()
            {
              switch (type)
              {
                case Type::Deals:
                  searchMarks.SetSales(sortedFeatures, true /* hasSale */);
                  break;
                case Type::Availability:
                  searchMarks.SetPreparingState(sortedFeatures, false /* isPreparing */);
                  break;
              }
            });

            // TODO(a): to add SetPrices method into search marks.
            // if (!pricesFormatted.empty())
            //   m_searchMarks.SetPrices(sortedFeatures, pricesFormatted)
          }
          cb(apiParams, sortedFeatures);
        }
      };

    tasksInternal.emplace_back(type, move(paramsInternal));
  }

  return tasksInternal;
}

filter::TasksRawInternal MakeInternalTasks(filter::Tasks const & filterTasks,
                                           SearchMarks & searchMarks)
{
  using namespace booking::filter;

  TasksRawInternal tasksInternal;

  for (auto const & task : filterTasks)
  {
    auto const type = task.m_type;
    auto const & apiParams = task.m_filterParams.m_apiParams;
    auto const & cb = task.m_filterParams.m_callback;

    if (apiParams->IsEmpty())
      continue;

    ParamsRawInternal paramsInternal
      {
        apiParams,
        [&searchMarks, type, apiParams, cb](auto && sortedFeatures, auto && extras)
        {
          if (sortedFeatures.empty())
            return;

          // TODO(a): add price formatting for array.
          // auto const pricesFormatted = FormatPrices(extras);

          GetPlatform().RunTask(Platform::Thread::Gui, [&searchMarks, type, sortedFeatures]()
          {
            switch (type)
            {
              case Type::Deals:
                searchMarks.SetSales(sortedFeatures, true /* hasSale */);
                break;
              case Type::Availability:
                searchMarks.SetPreparingState(sortedFeatures, false /* isPreparing */);
                break;
            }
            // TODO(a): to add SetPrices method into search marks.
            // if (!pricesFormatted.empty())
            //   m_searchMarks.SetPrices(sortedFeatures, pricesFormatted)
          });
          cb(apiParams, sortedFeatures);
        }
      };

    tasksInternal.emplace_back(type, move(paramsInternal));
  }

  return tasksInternal;
}
}  // namespace booking
