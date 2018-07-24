#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <string>

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".poly"

namespace borders
{
  using Region = m2::RegionD;
  using RegionsContainer = m4::Tree<Region>;

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

    RegionsContainer m_regions;
    std::string m_name;
    mutable int m_index;
  };

  using CountriesContainer = m4::Tree<CountryPolygons>;

  bool LoadCountriesList(std::string const & baseDir, CountriesContainer & countries);

  void GeneratePackedBorders(std::string const & baseDir);
  void UnpackBorders(std::string const & baseDir, std::string const & targetDir);
  bool GetBordersRect(std::string const & baseDir, std::string const & country, m2::RectD & bordersRect);
} // namespace borders
