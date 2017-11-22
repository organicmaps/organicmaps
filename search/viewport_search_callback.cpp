#include "search/viewport_search_callback.hpp"

#include "search/result.hpp"

#include "base/assert.hpp"

namespace search
{
ViewportSearchCallback::ViewportSearchCallback(Delegate & delegate, OnResults const & onResults)
  : m_delegate(delegate)
  , m_onResults(onResults)
  , m_hotelsModeSet(false)
  , m_firstCall(true)
  , m_lastResultsSize(0)
{
}

void ViewportSearchCallback::operator()(Results const & results)
{
  ASSERT_LESS_OR_EQUAL(m_lastResultsSize, results.GetCount(), ());
  m_hotelsClassif.Add(results.begin() + m_lastResultsSize, results.end());
  m_lastResultsSize = results.GetCount();

  if (!m_hotelsModeSet && m_hotelsClassif.IsHotelResults())
  {
    m_delegate.SetHotelDisplacementMode();
    m_hotelsModeSet = true;
  }

  // We need to clear old results and show a new bunch of results when
  // the search is completed normally (note that there may be empty
  // set of results), or when the search is not completed and there is
  // something in results.
  //
  // We don't need to clear old results or show current results if the
  // search is cancelled, because:

  // * current request is cancelled because of the next
  // search-in-viewport request - in this case it is the
  // responsibility of the next search request to clean up old results
  // and there is no need to clean up current results. We don't want
  // to show blinking results.
  //
  // * search in viewport may be cancelled completely - it is the
  // responsibility of the user of this class to handle this case and
  // clean up results.
  if (results.IsEndedNormal() || (!results.IsEndMarker() && results.GetCount() != 0))
  {
    auto & delegate = m_delegate;
    bool const firstCall = m_firstCall;

    m_delegate.RunUITask([&delegate, firstCall, results]() {
      if (!delegate.IsViewportSearchActive())
        return;

      if (firstCall)
        delegate.ClearViewportSearchResults();

      delegate.ShowViewportSearchResults(results);
    });
  }

  if (m_onResults)
    m_onResults(results);

  m_firstCall = false;
}
}  // namespace search
