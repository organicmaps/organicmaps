#include "map/viewport_search_callback.hpp"

#include "search/result.hpp"

#include "base/assert.hpp"

namespace
{
booking::filter::Types FillBookingFilterTypes(search::Results const & results,
                                              booking::filter::Tasks const & tasks)
{
  using namespace booking::filter;
  Types types;
  for (auto const & task : tasks)
  {
    switch (task.m_type)
    {
    case Type::Deals:
      if (results.GetType() == search::Results::Type::Hotels)
        types.emplace_back(Type::Deals);
      break;
    case Type::Availability:
      types.emplace_back(Type::Availability);
      break;
    }
  }

  return types;
}
}  // namespace

namespace search
{
ViewportSearchCallback::ViewportSearchCallback(m2::RectD const & viewport, Delegate & delegate,
                                               booking::filter::Tasks const & bookingFilterTasks,
                                               OnResults const & onResults)
  : m_viewport(viewport)
  , m_delegate(delegate)
  , m_onResults(onResults)
  , m_firstCall(true)
  , m_lastResultsSize(0)
  , m_bookingFilterTasks(bookingFilterTasks)
{
}

void ViewportSearchCallback::operator()(Results const & results)
{
  ASSERT_LESS_OR_EQUAL(m_lastResultsSize, results.GetCount(), ());

  if (results.GetType() == Results::Type::Hotels)
    m_delegate.SetHotelDisplacementMode();

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
  if (results.IsEndedNormal() || (!results.IsEndMarker() && results.GetCount() != 0))
  {
    auto & delegate = m_delegate;
    bool const firstCall = m_firstCall;

    auto const types = FillBookingFilterTypes(results, m_bookingFilterTasks);

    auto const lastResultsSize = m_lastResultsSize;
    m_delegate.RunUITask([&delegate, firstCall, types, results, lastResultsSize]() {
      if (!delegate.IsViewportSearchActive())
        return;

      if (types.empty())
      {
        delegate.ShowViewportSearchResults(results.begin() + lastResultsSize, results.end(),
                                           firstCall);
      }
      else
      {
        delegate.ShowViewportSearchResults(results.begin() + lastResultsSize, results.end(),
                                           firstCall, types);
      }
    });
  }

  if (results.IsEndedNormal() && results.GetType() == Results::Type::Hotels &&
      !m_bookingFilterTasks.IsEmpty())
  {
    if (m_viewport.IsValid())
      m_delegate.FilterAllHotelsInViewport(m_viewport, m_bookingFilterTasks);

    m_delegate.FilterResultsForHotelsQuery(m_bookingFilterTasks, results, true /* inViewport */);
  }

  m_lastResultsSize = results.GetCount();
  m_firstCall = false;

  if (m_onResults)
    m_onResults(results);
}
}  // namespace search
