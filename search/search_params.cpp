#include "search/search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/assert.hpp"

#include <sstream>

using namespace std;

namespace search
{
m2::PointD SearchParams::GetPositionMercator() const
{
  ASSERT(IsValidPosition(), ());
  return MercatorBounds::FromLatLon(*m_position);
}

ms::LatLon SearchParams::GetPositionLatLon() const
{
  ASSERT(IsValidPosition(), ());
  return *m_position;
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return m_query == rhs.m_query && m_inputLocale == rhs.m_inputLocale &&
         IsValidPosition() == rhs.IsValidPosition() && m_mode == rhs.m_mode;
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
