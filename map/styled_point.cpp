#include "map/styled_point.hpp"


graphics::DisplayList * StyledPoint::GetDisplayList(UserMarkDLCache * cache) const
{
  UserMarkContainer const * container = GetContainer();
  UserMarkDLCache::Key const key = GetStyle().empty() ? container->GetDefaultKey()
                                                      : UserMarkDLCache::Key(GetStyle(),
                                                                             graphics::EPosAbove,
                                                                             container->GetDepth());
  return cache->FindUserMark(key);
}

double StyledPoint::GetAnimScaleFactor() const
{
  // Matches the behaviour for non-custom drawables. The only caller
  // of ::DrawUserMark is UserMarkContainer::Draw and it always passes
  // this value.
  return 1.0;
}

m2::PointD const & StyledPoint::GetPixelOffset() const
{
  static m2::PointD const s_centre(0.0, 0.0);
  static m2::PointD const s_offset(0.0, 3.0);

  return GetStyle().empty() ? s_centre : s_offset;
}

static char const * s_arrSupportedColors[] =
{
  "placemark-red", "placemark-blue", "placemark-purple", "placemark-yellow",
  "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"
};

namespace style
{
  string GetSupportedStyle(string const & s, string const & context, string const & fallback)
  {
    if (s.empty())
      return fallback;

    for (size_t i = 0; i < ARRAY_SIZE(s_arrSupportedColors); ++i)
      if (s == s_arrSupportedColors[i])
        return s;

    // Not recognized symbols are replaced with default one
    LOG(LWARNING, ("Icon", s, "for point", context, "is not supported"));
    return fallback;
  }

  string GetDefaultStyle()
  {
    return s_arrSupportedColors[0];
  }

} // namespace style
