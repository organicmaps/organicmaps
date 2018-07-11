#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <string>

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".poly"

namespace borders
{
  typedef m2::RegionD Region;
  typedef m4::Tree<Region> RegionsContainerT;

  struct CountryPolygons
  {
    CountryPolygons(std::string const & name = "") : m_name(name), m_index(-1) {}

    bool IsEmpty() const { return m_regions.IsEmpty(); }
    void Clear()
    {
      m_regions.Clear();
      m_name.clear();
      m_index = -1;
    }

    RegionsContainerT m_regions;
    std::string m_name;
    mutable int m_index;
  };

  typedef m4::Tree<CountryPolygons> CountriesContainerT;

  bool LoadCountriesList(std::string const & baseDir, CountriesContainerT & countries);

  void GeneratePackedBorders(std::string const & baseDir);
  void UnpackBorders(std::string const & baseDir, std::string const & targetDir);
  bool GetBordersRect(std::string const & baseDir, std::string const & country, m2::RectD & bordersRect);
}
