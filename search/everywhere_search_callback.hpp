#pragma once

#include "search/everywhere_search_params.hpp"
#include "search/result.hpp"

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

    virtual bool IsLocalAdsCustomer(Result const & result) const = 0;
  };

  using OnResults = EverywhereSearchParams::OnResults;

  EverywhereSearchCallback(Delegate & delegate, OnResults onResults);

  void operator()(Results const & results);

private:
  Delegate & m_delegate;
  OnResults m_onResults;
  std::vector<bool> m_isLocalAdsCustomer;
};
}  // namespace search
