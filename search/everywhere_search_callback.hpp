#pragma once

#include "search/result.hpp"
#include "search/search_params.hpp"

namespace search
{
// An on-results-callback that should be used for interactive search.
//
// *NOTE* the class is NOT thread safe.
class EverywhereSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void MarkLocalAdsCustomer(Result & result) const = 0;
  };

  using OnResults = SearchParams::TOnResults;

  EverywhereSearchCallback(Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;
  Results m_results;
};
}  // namespace search
