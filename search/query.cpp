#include "query.hpp"
#include "../base/utf8_string.hpp"

namespace search1
{

Query::Query(string const & query)
{
  utf8_string::Split(query, m_Keywords, &utf8_string::IsSearchDelimiter);
  if (!query.empty() && !utf8_string::IsSearchDelimiter(query[query.size() - 1]))
  {
    m_Prefix.swap(m_Keywords.back());
    m_Keywords.pop_back();
  }
}



}
