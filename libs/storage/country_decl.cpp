#include "storage/country_decl.hpp"

#include <cstddef>

namespace storage
{
// static
CountryDef::Overlap CountryDef::IsIntersectOrInside(m2::RectD const & testRect, m2::RectD const & countryRect)
{
  auto const check = [&testRect](m2::RectD const & cr)
  {
    if (testRect.IsIntersect(cr))
      return testRect.IsRectInside(cr) ? Overlap::INSIDE : Overlap::INTERSECT;
    return Overlap::NONE;
  };

  auto res = check(countryRect);
  if (res != Overlap::NONE)
    return res;
  res = check({countryRect.minX() - 360.0, countryRect.minY(), countryRect.maxX() - 360.0, countryRect.maxY()});
  if (res != Overlap::NONE)
    return res;
  return check({countryRect.minX() + 360.0, countryRect.minY(), countryRect.maxX() + 360.0, countryRect.maxY()});
}

// static
void CountryInfo::FileName2FullName(std::string & fName)
{
  size_t const i = fName.find('_');
  if (i != std::string::npos)
  {
    // replace '_' with ", "
    fName[i] = ',';
    fName.insert(i + 1, " ");
  }
}

// static
void CountryInfo::FullName2GroupAndMap(std::string const & fName, std::string & group, std::string & map)
{
  size_t pos = fName.find(',');
  if (pos == std::string::npos)
  {
    map = fName;
    group.clear();
  }
  else
  {
    map = fName.substr(pos + 2);
    group = fName.substr(0, pos);
  }
}
}  // namespace storage
