#pragma once

#include "../geometry/region2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../std/fstream.hpp"
#include "../std/string.hpp"

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".poly"
#define POLYGONS_FILE "polygons.lst"

namespace borders
{
  typedef m2::RegionD Region;
  typedef m4::Tree<Region> RegionsContainerT;

  struct CountryPolygons
  {
    CountryPolygons(string const & name = "") : m_name(name), m_index(-1) {}

    bool IsEmpty() const { return m_regions.IsEmpty(); }
    void Clear()
    {
      m_regions.Clear();
      m_name.clear();
      m_index = -1;
    }

    RegionsContainerT m_regions;
    string m_name;
    mutable int m_index;
  };

  typedef m4::Tree<CountryPolygons> CountriesContainerT;

  /// @return false if borderFile can't be opened
  bool LoadBorders(string const & borderFile, vector<m2::RegionD> & outBorders);

  bool LoadCountriesList(string const & baseDir, CountriesContainerT & countries);

  template <class ToDo>
  void ForEachCountry(string const & baseDir, ToDo & toDo)
  {
    ifstream stream((baseDir + POLYGONS_FILE).c_str());
    string line;

    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      // in polygons file every country is a separate string
      toDo(line);
      toDo.Finish();
    }
  }
}
