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
void SortTransform(search::Results const & results, bool inViewport,
                   std::vector<Extras> const & extras, std::vector<FeatureID> & filteredOut,
                   std::vector<FeatureID> & available, std::vector<std::string> & prices)
{
  CHECK(extras.empty() || (results.GetCount() == extras.size()), ());

  std::vector<size_t> order(results.GetCount());
  std::iota(order.begin(), order.end(), 0);

  std::sort(order.begin(), order.end(),
      [&results](size_t lhs, size_t rhs) -> bool
      {
        return results[lhs].GetFeatureID() < results[rhs].GetFeatureID();
      });

  PriceFormatter formatter;
  for (size_t i : order)
  {
    if (results[i].IsRefusedByFilter())
    {
      if (inViewport)
        filteredOut.push_back(results[i].GetFeatureID());
    }
    else
    {
      available.emplace_back(results[i].GetFeatureID());

      if (inViewport)
      {
        auto priceFormatted = formatter.Format(extras[i].m_price, extras[i].m_currency);
        prices.push_back(std::move(priceFormatted));
      }
    }
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

    ParamsInternal paramsInternal{
        apiParams,
        [&searchMarks, type, apiParams, cb, inViewport](ResultInternal<search::Results> && result)
        {
          std::vector<FeatureID> filteredOut;
          std::vector<FeatureID> available;
          std::vector<std::string> prices;
          SortTransform(result.m_passedFilter, inViewport, result.m_extras, filteredOut, available,
                        prices);

          if (inViewport)
          {
            std::vector<FeatureID> unavailable;  // filtered out by online filters
            if (type == Type::Availability)
            {
              unavailable.reserve(result.m_filteredOut.GetCount());
              for (auto const & filtered : result.m_filteredOut)
                unavailable.push_back(filtered.GetFeatureID());
              std::sort(unavailable.begin(), unavailable.end());
            }

            GetPlatform().RunTask(
                Platform::Thread::Gui,
                [&searchMarks, type, filteredOut = std::move(filteredOut), available,
                 prices = std::move(prices), unavailable = std::move(unavailable)]() mutable
                {
                  switch (type)
                  {
                  case Type::Deals:
                    searchMarks.SetSales(available, true /* hasSale */);
                    break;
                  case Type::Availability:
                    searchMarks.SetPreparingState(filteredOut, false /* isPreparing */);
                    searchMarks.SetUnavailable(filteredOut, "booking_map_component_filters");

                    searchMarks.SetPreparingState(available, false /* isPreparing */);
                    searchMarks.SetPrices(available, std::move(prices));

                    searchMarks.SetPreparingState(unavailable, false /* isPreparing */);
                    searchMarks.SetUnavailable(unavailable, "booking_map_component_availability");
                    break;
                  }
                });
          }
          cb(apiParams, available);
        }};

    tasksInternal.emplace_back(type, std::move(paramsInternal));
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
          auto & sortedFeatures = result.m_passedFilter;
          auto & extras = result.m_extras;

          CHECK_EQUAL(sortedFeatures.size(), extras.size(), ());

          std::vector<FeatureID> filteredOut;  // filtered out by offline filters
          std::vector<FeatureID> available;
          std::vector<std::string> prices;
          PriceFormatter formatter;
          for (size_t i = 0; i < sortedFeatures.size(); ++i)
          {
            if (searchMarks.IsUnavailable(sortedFeatures[i]))
            {
              filteredOut.push_back(std::move(sortedFeatures[i]));
            }
            else
            {
              available.emplace_back(std::move(sortedFeatures[i]));
              prices.emplace_back(formatter.Format(extras[i].m_price, extras[i].m_currency));
            }
          }

          auto & unavailable = result.m_filteredOut;

          GetPlatform().RunTask(
              Platform::Thread::Gui,
              [&searchMarks, type, filteredOut = std::move(filteredOut), available,
               prices = std::move(prices), unavailable = std::move(unavailable)]() mutable
              {
                switch (type)
                {
                case Type::Deals:
                  searchMarks.SetSales(available, true /* hasSale */);
                  break;
                case Type::Availability:
                  searchMarks.SetPreparingState(filteredOut, false /* isPreparing */);
                  // do not change unavailability reason

                  searchMarks.SetPreparingState(available, false /* isPreparing */);
                  searchMarks.SetPrices(available, std::move(prices));

                  searchMarks.SetPreparingState(unavailable, false /* isPreparing */);
                  searchMarks.SetUnavailable(unavailable, "booking_map_component_availability");
                  break;
                }
              });
          cb(apiParams, available);
        }
      };

    tasksInternal.emplace_back(type, std::move(paramsInternal));
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
