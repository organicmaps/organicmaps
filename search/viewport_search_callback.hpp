#pragma once

#include "search/hotels_classifier.hpp"
#include "search/search_params.hpp"

#include <functional>

namespace search
{
class Results;

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
    virtual void ShowViewportSearchResults(Results const & results) = 0;
    virtual void ClearViewportSearchResults() = 0;
  };

  using OnResults = SearchParams::OnResults;

  ViewportSearchCallback(Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;

  bool m_hotelsModeSet;
  bool m_firstCall;

  HotelsClassifier m_hotelsClassif;
  size_t m_lastResultsSize;
};
}  // namespace search
