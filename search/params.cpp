#include "params.hpp"

#include "../coding/multilang_utf8_string.hpp"


namespace search
{

SearchParams::SearchParams() : m_searchMode(ALL), m_forceSearch(false), m_validPos(false)
{
}

void SearchParams::SetPosition(double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;
  m_validPos = true;
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return (m_query == rhs.m_query &&
          m_inputLanguage == rhs.m_inputLanguage &&
          m_validPos == rhs.m_validPos &&
          m_searchMode == rhs.m_searchMode);
}

string DebugPrint(SearchParams const & params)
{
  ostringstream stream;
  stream << "search::SearchParams: Query = " << params.m_query << " Mode = " << params.m_searchMode;
  return stream.str();
}

} // namespace search
