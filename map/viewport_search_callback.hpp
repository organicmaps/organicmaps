#pragma once

#include "map/booking_filter_params.hpp"

#include "search/hotels_classifier.hpp"
#include "search/result.hpp"
#include "search/search_params.hpp"

#include <functional>

namespace search
{
// An on-results-callback that should be used for interactive search.
//
// *NOTE* the class is NOT thread safe.
class ViewportSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> fn) = 0;
    virtual void SetHotelDisplacementMode() = 0;
    virtual bool IsViewportSearchActive() const = 0;
    virtual void ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end,
                                           bool clear) = 0;
    virtual void ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end,
                                           bool clear, booking::filter::Types types) = 0;
    virtual void FilterResultsForHotelsQuery(booking::filter::Tasks const & filterTasks,
                                             search::Results const & results, bool inViewport) = 0;
  };

  using OnResults = SearchParams::OnResults;

  ViewportSearchCallback(Delegate & delegate, booking::filter::Tasks const & bookingFilterTasks,
                         OnResults const & onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;

  bool m_firstCall;
  size_t m_lastResultsSize;
  booking::filter::Tasks m_bookingFilterTasks;
};
}  // namespace search
