#include "topography_generator/isolines_utils.hpp"

#include "generator/srtm_parser.hpp"

#include "storage/storage_defines.hpp"

#include "base/file_name_utils.hpp"

namespace topography_generator
{
std::string const kIsolinesExt = ".isolines";

std::string GetIsolinesTileBase(int bottomLat, int leftLon)
{
  auto const centerPoint = ms::LatLon(bottomLat + 0.5, leftLon + 0.5);
  return generator::SrtmTile::GetBase(centerPoint);
}

std::string GetIsolinesFilePath(int bottomLat, int leftLon, std::string const & dir)
{
  auto const fileName = GetIsolinesTileBase(bottomLat, leftLon);
  return base::JoinPath(dir, fileName + kIsolinesExt);
}

std::string GetIsolinesFilePath(storage::CountryId const & countryId, std::string const & dir)
{
  return base::JoinPath(dir, countryId + kIsolinesExt);
}
}  // namespace topography_generator
