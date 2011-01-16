#pragma once

#include "../../geometry/region2d.hpp"

#include "../../std/string.hpp"
#include "../../std/vector.hpp"

namespace kml
{
  typedef m2::RegionU Region;
  typedef std::vector<Region> RegionsContainerT;

  struct CountryPolygons
  {
    CountryPolygons(string const & name = "") : m_name(name) {}
    RegionsContainerT m_regions;
    string m_name;
    /// limit rect for all country polygons
    m2::RectD m_rect;
  };

  typedef vector<CountryPolygons> CountriesContainerT;

  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries);
}
