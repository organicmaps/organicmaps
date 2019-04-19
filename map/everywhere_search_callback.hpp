#pragma once

#include "map/booking_filter_params.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/search_product_info.hpp"

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
    virtual void FilterResultsForHotelsQuery(booking::filter::Tasks const & filterTasks,
                                             search::Results const & results, bool inViewport) = 0;
  };

  EverywhereSearchCallback(Delegate & hotelsDelegate, ProductInfo::Delegate & productInfoDelegate,
                           booking::filter::Tasks const & bookingFilterTasks,
                           EverywhereSearchParams::OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_hotelsDelegate;
  ProductInfo::Delegate & m_productInfoDelegate;
  EverywhereSearchParams::OnResults m_onResults;
  std::vector<ProductInfo> m_productInfo;
  booking::filter::Tasks m_bookingFilterTasks;
};
}  // namespace search
