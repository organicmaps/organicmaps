#include "map/api_mark_point.hpp"

#include "base/logging.hpp"

namespace style
{

char const * kSupportedColors[] = {"placemark-red",    "placemark-blue",  "placemark-purple",
                                   "placemark-yellow", "placemark-pink",  "placemark-brown",
                                   "placemark-green",  "placemark-orange"};

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

ApiMarkPoint::ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{}

ApiMarkPoint::ApiMarkPoint(string const & name, string const & id, string const & style,
                           m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container),
    m_name(name),
    m_id(id),
    m_style(style)
{}

string ApiMarkPoint::GetSymbolName() const
{
  return m_style.empty() ? "api-result" : m_style;
}

UserMark::Type ApiMarkPoint::GetMarkType() const
{
  return UserMark::Type::API;
}

m2::PointD const & ApiMarkPoint::GetPixelOffset() const
{
  static m2::PointD const s_centre(0.0, 0.0);
  static m2::PointD const s_offset(0.0, 3.0);

  return m_style.empty() ? s_centre : s_offset;
}
