#pragma once

#include "map/viewport_search_params.hpp"

#include "search/result.hpp"

#include "geometry/rect2d.hpp"

#include <functional>

namespace search
{
/// @brief An on-results-callback that should be used for interactive search.
/// @note NOT thread safe.
class ViewportSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> fn) = 0;
    virtual bool IsViewportSearchActive() const = 0;
    virtual void ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end, bool clear) = 0;
  };

  using OnResults = ViewportSearchParams::OnCompleted;

  ViewportSearchCallback(m2::RectD const & viewport, Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  m2::RectD m_viewport;

  Delegate & m_delegate;
  OnResults m_onResults;

  bool m_firstCall;
  size_t m_lastResultsSize;
};
}  // namespace search
