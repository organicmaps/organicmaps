#pragma once

#include "../../geometry/region2d.hpp"
#include "../../geometry/tree4d.hpp"

#include "../../std/string.hpp"


namespace kml
{
  typedef m2::RegionD Region;
  typedef m4::Tree<Region> RegionsContainerT;

  struct CountryPolygons
  {
    CountryPolygons(string const & name = "") : m_name(name), m_index(-1) {}

    RegionsContainerT m_regions;
    string m_name;
    mutable int m_index;
  };

  typedef m4::Tree<CountryPolygons> CountriesContainerT;

  /// @param[in] simplifyCountriesLevel if positive, used as a level for simplificator
  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries,
                         int simplifyCountriesLevel = -1);
}
