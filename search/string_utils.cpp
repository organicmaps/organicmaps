#include "search/string_utils.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"

namespace search
{

QueryString MakeQueryString(std::string s)
{
  QueryString qs;
  qs.m_query = std::move(s);

  Delimiters delims;
  auto const uniString = NormalizeAndSimplifyString(qs.m_query);
  SplitUniString(uniString, base::MakeBackInsertFunctor(qs.m_tokens), delims);

  if (!qs.m_tokens.empty() && !delims(uniString.back()))
  {
    qs.m_prefix = qs.m_tokens.back();
    qs.m_tokens.pop_back();
  }

  return qs;
}

}  // namespace search
