#include "map/viewport_search_callback.hpp"

#include "search/result.hpp"

#include "base/assert.hpp"

namespace search
{
ViewportSearchCallback::ViewportSearchCallback(m2::RectD const & viewport, Delegate & delegate, OnResults onResults)
  : m_viewport(viewport)
  , m_delegate(delegate)
  , m_onResults(std::move(onResults))
  , m_firstCall(true)
  , m_lastResultsSize(0)
{
  // m_onResults may be empty (current Android)
}

void ViewportSearchCallback::operator()(Results const & results)
{
  ASSERT_LESS_OR_EQUAL(m_lastResultsSize, results.GetCount(), ());

  // We need to clear old results and show a new bunch of results when
  // the search is completed normally (note that there may be empty
  // set of results), or when the search is not completed and there is
  // something in results.
  //
  // We don't need to clear old results or show current results if the
  // search is cancelled, because:
  //
  // * current request is cancelled because of the next
  // search-in-viewport request - in this case it is the
  // responsibility of the next search request to clean up old results
  // and there is no need to clean up current results. We don't want
  // to show blinking results.
  //
  // * search in viewport may be cancelled completely - it is the
  // responsibility of the user of this class to handle this case and
  // clean up results.

  auto & delegate = m_delegate;
  m_delegate.RunUITask([&delegate, results, onResults = m_onResults,
                       firstCall = m_firstCall, lastResultsSize = m_lastResultsSize]() mutable
  {
    if (delegate.IsViewportSearchActive() &&
        (results.IsEndedNormal() || (!results.IsEndMarker() && results.GetCount() != 0)))
    {
      /// @todo This code relies on fact that results order is *NOT* changing with every new batch.
      /// I can't say for sure is it true or not, but very optimistic :)
      /// Much easier to clear previous marks and make new ones.
      delegate.ShowViewportSearchResults(results.begin() + lastResultsSize, results.end(), firstCall);
    }

    if (results.IsEndMarker() && onResults)
      onResults(std::move(results));
  });

  m_lastResultsSize = results.GetCount();
  m_firstCall = false;
}
}  // namespace search
