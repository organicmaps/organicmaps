#pragma once

#include "map/booking_filter_params.hpp"
#include "map/everywhere_search_params.hpp"

#include "search/result.hpp"

#include <functional>
#include <vector>

namespace search
{
// An on-results-callback that should be used for search over all
// maps.
//
// *NOTE* the class is NOT thread safe.
class EverywhereSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual ProductInfo GetProductInfo(Result const & result) const = 0;
    virtual void FilterSearchResultsOnBooking(booking::filter::Tasks const & filterTasks,
                                              search::Results const & results, bool inViewport) = 0;
  };

  EverywhereSearchCallback(Delegate & delegate, booking::filter::Tasks const & bookingFilterTasks,
                           EverywhereSearchParams::OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  EverywhereSearchParams::OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
  booking::filter::Tasks m_bookingFilterTasks;
};
}  // namespace search
