#pragma once

#include "storage/storage_defines.hpp"

#include "coding/geometry_coding.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

#include <string>

#include <string>
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

/// @return false if borderFile can't be opened
bool LoadBorders(std::string const & borderFile, std::vector<m2::RegionD> & outBorders);

bool GetBordersRect(std::string const & baseDir, std::string const & country,
                    m2::RectD & bordersRect);

bool LoadCountriesList(std::string const & baseDir, CountriesContainer & countries);

void GeneratePackedBorders(std::string const & baseDir);

template <typename Source>
std::vector<std::vector<m2::PointD>> ReadPolygonsOfOneBorder(Source & src)
{
  auto const count = ReadVarUint<uint32_t>(src);
  std::vector<std::vector<m2::PointD>> result(count);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result[i] = std::move(points);
  }

  return result;
}

void DumpBorderToPolyFile(std::string const & filePath, storage::CountryId const & mwmName,
                          std::vector<std::vector<m2::PointD>> const & polygons);
void UnpackBorders(std::string const & baseDir, std::string const & targetDir);
}  // namespace borders
