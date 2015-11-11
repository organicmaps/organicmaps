#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

#include "search/result.hpp"

#include "indexer/feature.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

namespace style
{
// Fixes icons which are not supported by MapsWithMe.
string GetSupportedStyle(string const & s, string const & context, string const & fallback);
// Default icon.
string GetDefaultStyle();
}  // namespace style

class StyledPoint : public ICustomDrawable
{
public:
  StyledPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : ICustomDrawable(ptOrg, container)
  {
  }

  // ICustomDrawable overrides:
  graphics::DisplayList * GetDisplayList(UserMarkDLCache * cache) const override;
  double GetAnimScaleFactor() const override;
  m2::PointD const & GetPixelOffset() const override;

  /// @return name of icon, or empty string for plain circle.
  virtual string const & GetStyle() const = 0;
};
