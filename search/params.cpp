#include "params.hpp"

#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"


namespace search
{
SearchParams::SearchParams()
  : m_searchRadiusM(-1.0)
  , m_mode(Mode::Everywhere)
  , m_forceSearch(false)
  , m_validPos(false)
  , m_suggestsEnabled(true)
{
}

void SearchParams::SetPosition(double lat, double lon)
{
  m_lat = lat;
  m_lon = lon;
  m_validPos = true;
}

bool SearchParams::GetSearchRect(m2::RectD & rect) const
{
  if (IsSearchAroundPosition())
  {
    rect = MercatorBounds::MetresToXY(m_lon, m_lat, m_searchRadiusM);
    return true;
  }
  return false;
}

bool SearchParams::IsEqualCommon(SearchParams const & rhs) const
{
  return (m_query == rhs.m_query &&
          m_inputLocale == rhs.m_inputLocale &&
          m_validPos == rhs.m_validPos &&
          m_mode == rhs.m_mode &&
          m_searchRadiusM == rhs.m_searchRadiusM);
}

string DebugPrint(SearchParams const & params)
{
  ostringstream ss;
  ss << "{ SearchParams: Query = " << params.m_query << ", Locale = " << params.m_inputLocale
     << ", Mode = " << DebugPrint(params.m_mode) << " }";
  return ss.str();
}
} // namespace search
