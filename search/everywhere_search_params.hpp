#pragma once

#include "search/hotels_filter.hpp"
#include "search/result.hpp"
#include "search/search_params.hpp"

#include "std/functional.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
struct EverywhereSearchParams
{
  using OnResults =
      function<void(Results const & results, vector<bool> const & isLocalAdsCustomer)>;

  string m_query;
  string m_inputLocale;
  shared_ptr<hotels_filter::Rule> m_hotelsFilter;

  OnResults m_onResults;
};
}  // namespace search
