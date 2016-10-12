#pragma once

#include "search/hotels_filter.hpp"
#include "search/search_params.hpp"

#include "std/string.hpp"

namespace search
{
struct EverywhereSearchParams
{
  string m_query;
  string m_inputLocale;
  shared_ptr<hotels_filter::Rule> m_hotelsFilter;

  SearchParams::TOnResults m_onResults;
};
}  // namespace search
