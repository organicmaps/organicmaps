#include "map/booking_utils.hpp"

#include "map/search_mark.hpp"

#include "search/result.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/localization.hpp"

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
    PriceFormatter formatter;
    for (size_t i = 0; i < results.GetCount(); ++i)
    {
      if (results[i].IsRefusedByFilter())
        continue;

      auto priceFormatted = formatter.Format(extras[i].m_price, extras[i].m_currency);
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

void SetUnavailable(search::Results const & filteredOut, SearchMarks & searchMarks)
{
  if (filteredOut.GetCount() == 0)
    return;

  for (auto const & filtered : filteredOut)
  {
    searchMarks.SetUnavailable(filtered.GetFeatureID(), "booking_map_component_availability");
  }
}

void SetUnavailable(std::vector<FeatureID> const & filteredOut, SearchMarks & searchMarks)
{
  if (filteredOut.empty())
    return;

  for (auto const & filtered : filteredOut)
  {
    searchMarks.SetUnavailable(filtered, "booking_map_component_availability");
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
        [&searchMarks, type, apiParams, cb, inViewport](ResultInternal<search::Results> && result)
        {
          if (type == Type::Availability)
            SetUnavailable(result.m_filteredOut, searchMarks);

          auto & filteredResults = result.m_passedFilter;
          auto & extras = result.m_extras;

          if (filteredResults.GetCount() == 0)
            return;

          if (!inViewport)
            extras.clear();

          std::vector<FeatureID> sortedFeatures;
          std::vector<std::string> sortedPrices;

          SortTransform(filteredResults, extras, sortedFeatures, sortedPrices);

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
        [&searchMarks, type, apiParams, cb](ResultInternal<std::vector<FeatureID>> && result)
        {
          if (type == Type::Availability)
            SetUnavailable(result.m_filteredOut, searchMarks);

          auto & sortedFeatures = result.m_passedFilter;
          auto & extras = result.m_extras;

          if (sortedFeatures.empty())
            return;

          CHECK_EQUAL(sortedFeatures.size(), extras.size(), ());

          PriceFormatter formatter;
          std::vector<FeatureID> sortedAvailable;
          std::vector<std::string> orderedPrices;
          for (size_t i = 0; i < sortedFeatures.size(); ++i)
          {
            // Some hotels might be unavailable by offline filter.
            if (searchMarks.IsUnavailable(sortedFeatures[i]))
              continue;

            sortedAvailable.emplace_back(std::move(sortedFeatures[i]));
            orderedPrices.emplace_back(formatter.Format(extras[i].m_price, extras[i].m_currency));
          }

          GetPlatform().RunTask(Platform::Thread::Gui, [&searchMarks, type, sortedAvailable,
                                                        orderedPrices = std::move(orderedPrices)]() mutable
          {
            switch (type)
            {
              case Type::Deals:
                searchMarks.SetSales(sortedAvailable, true /* hasSale */);
                break;
              case Type::Availability:
                searchMarks.SetPreparingState(sortedAvailable, false /* isPreparing */);
                searchMarks.SetPrices(sortedAvailable, std::move(orderedPrices));
                break;
            }
          });
          cb(apiParams, sortedAvailable);
        }
      };

    tasksInternal.emplace_back(type, move(paramsInternal));
  }

  return tasksInternal;
}

std::string PriceFormatter::Format(double price, std::string const & currency)
{
  if (currency != m_currencyCode)
  {
    m_currencySymbol = platform::GetCurrencySymbol(currency);
    m_currencyCode = currency;
  }
  auto const priceStr = std::to_string(static_cast<uint64_t>(price));

  auto constexpr kComma = ',';
  size_t constexpr kCommaStep = 3;
  auto constexpr kEllipsizeSymbol = u8"\u2026";
  size_t constexpr kEllipsizeAfter = 7;
  size_t constexpr kNoEllipsizeLimit = 10;

  size_t const firstComma = priceStr.size() % kCommaStep;
  size_t commaCount = 0;
  size_t const fullLength = priceStr.size() + priceStr.size() / kCommaStep;
  bool const needEllipsize = fullLength > kNoEllipsizeLimit;
  std::string result;
  for (size_t i = 0; i < priceStr.size(); ++i)
  {
    if (i == firstComma + commaCount * kCommaStep)
    {
      if (i != 0)
        result += kComma;
      ++commaCount;
    }

    if (needEllipsize && result.size() > kEllipsizeAfter)
    {
      result += kEllipsizeSymbol;
      break;
    }

    result += priceStr[i];
  }

  result += " " + m_currencySymbol;

  return result;
}
}  // namespace booking
