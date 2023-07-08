#pragma once
#include "generator/srtm_parser.hpp"

#include "storage/storage_defines.hpp"

#include "indexer/feature_altitude.hpp"

#include "base/file_name_utils.hpp"

#include <string>

namespace topography_generator
{
using Altitude = geometry::Altitude;
Altitude constexpr kInvalidAltitude = geometry::kInvalidAltitude;

constexpr char const * const kIsolinesExt = ".isolines";

inline std::string GetIsolinesTileBase(int bottomLat, int leftLon)
{
  auto const centerPoint = ms::LatLon(bottomLat + 0.5, leftLon + 0.5);
  return generator::SrtmTile::GetBase(centerPoint);
}

inline std::string GetIsolinesFilePath(int bottomLat, int leftLon, std::string const & dir)
{
  auto const fileName = GetIsolinesTileBase(bottomLat, leftLon);
  return base::JoinPath(dir, fileName + kIsolinesExt);
}

inline std::string GetIsolinesFilePath(storage::CountryId const & countryId, std::string const & dir)
{
  return base::JoinPath(dir, countryId + kIsolinesExt);
}
}  // namespace topography_generator
