#include "search/search_params.hpp"

#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/assert.hpp"

namespace search
{
void SearchParams::SetPosition(double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;
  m_validPos = true;
}

m2::PointD SearchParams::GetPositionMercator() const
{
  ASSERT(IsValidPosition(), ());
  return MercatorBounds::FromLatLon(m_lat, m_lon);
}

ms::LatLon SearchParams::GetPositionLatLon() const
{
  ASSERT(IsValidPosition(), ());
  return ms::LatLon(m_lat, m_lon);
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return m_query == rhs.m_query && m_inputLocale == rhs.m_inputLocale &&
         m_validPos == rhs.m_validPos && m_mode == rhs.m_mode;
}

string DebugPrint(SearchParams const & params)
{
  ostringstream ss;
  ss << "{ SearchParams: Query = " << params.m_query << ", Locale = " << params.m_inputLocale
     << ", Mode = " << DebugPrint(params.m_mode) << " }";
  return ss.str();
}
}  // namespace search
