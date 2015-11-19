#include "map/styled_point.hpp"

#include "base/logging.hpp"

namespace
{
char const * kSupportedColors[] = {"placemark-red",    "placemark-blue",  "placemark-purple",
                                   "placemark-yellow", "placemark-pink",  "placemark-brown",
                                   "placemark-green",  "placemark-orange"};
}

namespace style
{

StyledPoint::StyledPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
}

m2::PointD const & StyledPoint::GetPixelOffset() const
{
  static m2::PointD const s_centre(0.0, 0.0);
  static m2::PointD const s_offset(0.0, 3.0);

  return GetStyle().empty() ? s_centre : s_offset;
}

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
}  // namespace style
