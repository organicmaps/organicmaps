#pragma once

#include "search/result.hpp"
#include "search/search_params.hpp"

#include "geometry/rect2d.hpp"

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
    virtual bool IsViewportSearchActive() const = 0;
    virtual void ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end,
                                           bool clear) = 0;
  };

  using OnResults = SearchParams::OnResults;

  ViewportSearchCallback(m2::RectD const & viewport, Delegate & delegate,
                         OnResults const & onResults);

  void operator()(Results const & results);

private:
  m2::RectD m_viewport;

  Delegate & m_delegate;
  OnResults m_onResults;

  bool m_firstCall;
  size_t m_lastResultsSize;
};
}  // namespace search
