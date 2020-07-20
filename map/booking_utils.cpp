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
void SortTransform(search::Results const & results, std::vector<Extras> const & extras,
                   std::vector<FeatureID> & sortedFeatures, std::vector<std::string> & sortedPrices)
{
  if (!extras.empty())
  {
    CHECK_EQUAL(results.GetCount(), extras.size(), ());

    std::vector<std::pair<FeatureID, std::string>> featuresWithPrices;
    featuresWithPrices.reserve(results.GetCount());
    for (size_t i = 0; i < results.GetCount(); ++i)
    {
      auto priceFormatted = FormatPrice(extras[i].m_price, extras[i].m_currency);
      featuresWithPrices.emplace_back(results[i].GetFeatureID(), std::move(priceFormatted));
    }

    std::sort(featuresWithPrices.begin(), featuresWithPrices.end(),
              base::LessBy(&std::pair<FeatureID, std::string>::first));

    sortedFeatures.reserve(featuresWithPrices.size());
    sortedPrices.reserve(featuresWithPrices.size());
    for (auto & item : featuresWithPrices)
    {
      sortedFeatures.emplace_back(std::move(item.first));
      sortedPrices.emplace_back(std::move(item.second));
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

          if (!inViewport)
            extras.clear();

          std::vector<FeatureID> sortedFeatures;
          std::vector<std::string> sortedPrices;

          SortTransform(std::move(results), std::move(extras), sortedFeatures, sortedPrices);

          if (inViewport)
          {
            GetPlatform().RunTask(Platform::Thread::Gui, [&searchMarks, type, sortedFeatures,
                                                          sortedPrices = std::move(sortedPrices)]() mutable
            {
              switch (type)
              {
                case Type::Deals:
                  searchMarks.SetSales(sortedFeatures, true /* hasSale */);
                  break;
                case Type::Availability:
                  searchMarks.SetPreparingState(sortedFeatures, false /* isPreparing */);
                  searchMarks.SetPrices(sortedFeatures, std::move(sortedPrices));
                  break;
              }
            });
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

          std::vector<std::string> sortedPrices;
          sortedPrices.reserve(extras.size());
          for (size_t i = 0; i < extras.size(); ++i)
            sortedPrices.emplace_back(FormatPrice(extras[i].m_price, extras[i].m_currency));

          GetPlatform().RunTask(Platform::Thread::Gui, [&searchMarks, type, sortedFeatures,
                                                        sortedPrices = std::move(sortedPrices)]() mutable
          {
            switch (type)
            {
              case Type::Deals:
                searchMarks.SetSales(sortedFeatures, true /* hasSale */);
                break;
              case Type::Availability:
                searchMarks.SetPreparingState(sortedFeatures, false /* isPreparing */);
                searchMarks.SetPrices(sortedFeatures, std::move(sortedPrices));
                break;
            }
          });
          cb(apiParams, sortedFeatures);
        }
      };

    tasksInternal.emplace_back(type, move(paramsInternal));
  }

  return tasksInternal;
}

std::string FormatPrice(double price, std::string const & currency)
{
  // TODO(a): add price formatting.
  return std::to_string(static_cast<uint32_t>(price)) + " " + currency;
}
}  // namespace booking
