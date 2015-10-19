#include "map/styled_point.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/fstream.hpp"
#include "std/algorithm.hpp"
#include "std/auto_ptr.hpp"


graphics::DisplayList * StyledPoint::GetDisplayList(UserMarkDLCache * cache) const
{
  return cache->FindUserMark(UserMarkDLCache::Key(GetStyle(), graphics::EPosAbove, GetContainer()->GetDepth()));
}

double StyledPoint::GetAnimScaleFactor() const
{
  return 1.0;
}

m2::PointD const & StyledPoint::GetPixelOffset() const
{
  static m2::PointD s_offset(0.0, 3.0);
  return s_offset;
}

static char const * s_arrSupportedColors[] =
{
  "placemark-red", "placemark-blue", "placemark-purple", "placemark-yellow",
  "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"
};

namespace style
{
  string GetSupportedStyle(string const & s, string const & context)
  {
    if (s.empty())
      return s_arrSupportedColors[0];

    for (size_t i = 0; i < ARRAY_SIZE(s_arrSupportedColors); ++i)
      if (s == s_arrSupportedColors[i])
        return s;

    // Not recognized symbols are replaced with default one
    LOG(LWARNING, ("Icon", s, "for point", context, "is not supported"));
    return s_arrSupportedColors[0];
  }

  string GetDefaultStyle()
  {
    return s_arrSupportedColors[0];
  }

} // namespace style
