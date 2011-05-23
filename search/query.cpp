#include "query.hpp"
#include "delimiters.hpp"

#include "../base/string_utils.hpp"

namespace search1
{

Query::Query(string const & query)
{
  search::Delimiters delims;
  strings::TokenizeIterator<search::Delimiters> iter(query, delims);
  while (iter)
  {
    if (iter.IsLast() && !delims(strings::LastUniChar(query)))
      m_prefix = *iter;
    else
      m_keywords.push_back(*iter);
  }
}

}
