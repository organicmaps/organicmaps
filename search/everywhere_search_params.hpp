#pragma once

#include "search/search_params.hpp"

#include "std/string.hpp"

namespace search
{
struct EverywhereSearchParams
{
  string m_query;
  string m_inputLocale;

  SearchParams::TOnResults m_onResults;
};
}  // namespace search
