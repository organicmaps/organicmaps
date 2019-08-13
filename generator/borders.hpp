#pragma once

#include "generator/feature_builder.hpp"

#include "storage/storage_defines.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".poly"

namespace borders
{
// The raw borders that we have somehow obtained (probably downloaded from
// the OSM and then manually tweaked, but the exact knowledge is lost) are
// stored in the BORDERS_DIR.
// These are the sources of the mwm borders: each file there corresponds
// to exactly one mwm and all mwms except for World and WorldCoasts must
// have a borders file to be generated from.
//
// The file format for raw borders is described at
//   https://wiki.openstreetmap.org/wiki/Osmosis/Polygon_Filter_File_Format
//
// The borders for all mwm files are shipped with the appilication in
// the mwm binary format for geometry data (see coding/geometry_coding.hpp).
// However, storing every single point turned out to take too much space,
// therefore the borders are simplified. This simplification may lead to
// unwanted consequences (for example, empty spaces may occur between mwms)
// but currently we do not take any action against them.

using Region = m2::RegionD;
using RegionsContainer = m4::Tree<Region>;

class CountryPolygons
{
public:
  CountryPolygons() = default;
  explicit CountryPolygons(std::string const & name, RegionsContainer const & regions)
    : m_name(name)
    , m_regions(regions)
  {
  }

  CountryPolygons(CountryPolygons && other) = default;
  CountryPolygons(CountryPolygons const & other) = default;

  CountryPolygons & operator=(CountryPolygons && other) = default;
  CountryPolygons & operator=(CountryPolygons const & other) = default;

  std::string const & GetName() const { return m_name; }
  bool IsEmpty() const { return m_regions.IsEmpty(); }
  void Clear()
  {
    m_regions.Clear();
    m_name.clear();
  }

  bool Contains(m2::PointD const & point) const
  {
    return m_regions.ForAnyInRect(m2::RectD(point, point), [&](auto const & rgn) {
      return rgn.Contains(point);
    });
  }

private:
  std::string m_name;
  RegionsContainer m_regions;
};

class CountriesContainer
{
public:
  CountriesContainer() = default;
  explicit CountriesContainer(m4::Tree<CountryPolygons> const & tree)
    : m_regionsTree(tree)
  {
  }

  template <typename ToDo>
  void ForEachInRect(m2::RectD const & rect, ToDo && toDo) const
  {
    m_regionsTree.ForEachInRect(rect, std::forward<ToDo>(toDo));
  }

  bool HasRegionByName(std::string const & name) const
  {
    return m_regionsTree.FindNode([&](auto const & countryPolygons) {
      return countryPolygons.GetName() == name;
    });
  }

  CountryPolygons const & GetRegionByName(std::string const & name) const
  {
    ASSERT(HasRegionByName(name), ());

    CountryPolygons const * country = nullptr;
    m_regionsTree.FindNode([&](auto const & countryPolygons) {
      if (countryPolygons.GetName() == name)
      {
        country = &countryPolygons;
        return true;
      }

      return false;
    });

    return *country;
  }

private:
  m4::Tree<CountryPolygons> m_regionsTree;
};

/// @return false if borderFile can't be opened
bool LoadBorders(std::string const & borderFile, std::vector<m2::RegionD> & outBorders);

bool GetBordersRect(std::string const & baseDir, std::string const & country,
                    m2::RectD & bordersRect);

bool LoadCountriesList(std::string const & baseDir, CountriesContainer & countries);

void GeneratePackedBorders(std::string const & baseDir);

template <typename Source>
std::vector<m2::RegionD> ReadPolygonsOfOneBorder(Source & src)
{
  auto const count = ReadVarUint<uint32_t>(src);
  std::vector<m2::RegionD> result(count);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result[i] = m2::RegionD(std::move(points));
  }

  return result;
}

void DumpBorderToPolyFile(std::string const & filePath, storage::CountryId const & mwmName,
                          std::vector<m2::RegionD> const & polygons);
void UnpackBorders(std::string const & baseDir, std::string const & targetDir);

class PackedBorders
{
public:
  static CountriesContainer const & GetOrCreate(std::string const & name);

private:
  static std::mutex m_mutex;
  static std::unordered_map<std::string, CountriesContainer> m_countries;
};
}  // namespace borders
