#include "search/search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"

#include <sstream>

using namespace std;

namespace search
{
bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return m_query == rhs.m_query && m_inputLocale == rhs.m_inputLocale &&
         static_cast<bool>(m_position) == static_cast<bool>(rhs.m_position) && m_mode == rhs.m_mode;
}

string DebugPrint(SearchParams const & params)
{
  ostringstream os;
  os << "SearchParams [";
  os << "query: " << params.m_query << ", ";
  os << "locale: " << params.m_inputLocale << ", ";
  os << "mode: " << DebugPrint(params.m_mode);
  os << "]";
  return os.str();
}
}  // namespace search
