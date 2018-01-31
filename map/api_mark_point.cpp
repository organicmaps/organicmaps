#include "map/api_mark_point.hpp"

#include "base/logging.hpp"

namespace style
{

char const * kSupportedColors[] = {"placemark-red",    "placemark-blue",    "placemark-purple",
                                   "placemark-yellow", "placemark-pink",    "placemark-brown",
                                   "placemark-green",  "placemark-orange",  "placemark-hotel"};

string GetSupportedStyle(string const & s, string const & context, string const & fallback)
{
  if (s.empty())
    return fallback;

  for (size_t i = 0; i < ARRAY_SIZE(kSupportedColors); ++i)
  {
    if (s == kSupportedColors[i])
      return s;
  }

  // Not recognized symbols are replaced with default one
  LOG(LWARNING, ("Icon", s, "for point", context, "is not supported"));
  return fallback;
}

string GetDefaultStyle() { return kSupportedColors[0]; }

} // style

ApiMarkPoint::ApiMarkPoint(m2::PointD const & ptOrg, UserMarkManager * manager)
  : UserMark(ptOrg, manager, UserMark::Type::API)
{}

ApiMarkPoint::ApiMarkPoint(string const & name, string const & id, string const & style,
                           m2::PointD const & ptOrg, UserMarkManager * manager)
  : UserMark(ptOrg, manager, UserMark::Type::API),
    m_name(name),
    m_id(id),
    m_style(style)
{}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> ApiMarkPoint::GetSymbolNames() const
{
  auto const name = m_style.empty() ? "api-result" : m_style;
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  return symbol;
}

m2::PointD ApiMarkPoint::GetPixelOffset() const
{
  return m_style.empty() ? m2::PointD(0.0, 0.0) : m2::PointD(0.0, 3.0);
}

void ApiMarkPoint::SetName(string const & name)
{
  SetDirty();
  m_name = name;
}

void ApiMarkPoint::SetApiID(string const & id)
{
  SetDirty();
  m_id = id;
}

void ApiMarkPoint::SetStyle(string const & style)
{
  SetDirty();
  m_style = style;
}
