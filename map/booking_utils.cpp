#include "map/booking_utils.hpp"

#include "map/search_mark.hpp"

#include "search/result.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/localization.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

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
    PriceFormatter formatter;
    for (size_t i = 0; i < results.GetCount(); ++i)
    {
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

void AppendUnavailable(search::Results const & filteredOut, SearchMarks & searchMarks)
{
  if (filteredOut.GetCount() == 0)
    return;

  std::string reason = platform::GetLocalizedString("booking_map_component_availability");
  for (auto const & filtered : filteredOut)
  {
    searchMarks.AppendUnavailable(filtered.GetFeatureID(), reason);
  }
}

void AppendUnavailable(std::vector<FeatureID> const & filteredOut, SearchMarks & searchMarks)
{
  if (filteredOut.empty())
    return;

  std::string reason = platform::GetLocalizedString("booking_map_component_availability");
  for (auto const & filtered : filteredOut)
  {
    searchMarks.AppendUnavailable(filtered, reason);
  }
}
}  // namespace

filter::TasksInternal MakeInternalTasks(search::Results const & results,
                                        filter::Tasks const & filterTasks,
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
        [results, &searchMarks, type, apiParams, cb, inViewport](ResultInternal<search::Results> && result)
        {
          if (type == Type::Availability)
            AppendUnavailable(result.m_filteredOut, searchMarks);

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

filter::TasksRawInternal MakeInternalTasks(std::vector<FeatureID> const & features,
                                           filter::Tasks const & filterTasks,
                                           SearchMarks & searchMarks)
{
  using namespace booking::filter;

  TasksRawInternal tasksInternal;

  ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());

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
        [features, &searchMarks, type, apiParams, cb](ResultInternal<std::vector<FeatureID>> && result)
        {
          if (type == Type::Availability)
            AppendUnavailable(result.m_filteredOut, searchMarks);

          auto & sortedFeatures = result.m_passedFilter;
          auto & extras = result.m_extras;

          ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());

          if (sortedFeatures.empty())
            return;

          std::vector<std::string> orderedPrices;
          orderedPrices.reserve(extras.size());
          PriceFormatter formatter;
          for (size_t i = 0; i < extras.size(); ++i)
            orderedPrices.emplace_back(formatter.Format(extras[i].m_price, extras[i].m_currency));

          std::vector<FeatureID> sortedAvailable;
          for (auto & id : sortedFeatures)
          {
            // Some hotels might be unavailable by offline filter.
            if (!searchMarks.IsUnavailable(id))
              sortedAvailable.emplace_back(std::move(id));
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
