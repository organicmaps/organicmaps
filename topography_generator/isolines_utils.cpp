#include "topography_generator/isolines_utils.hpp"

#include "generator/srtm_parser.hpp"

#include "base/file_name_utils.hpp"

namespace topography_generator
{
std::string const kIsolinesExt = ".isolines";

std::string GetIsolinesFilePath(int lat, int lon, std::string const & dir)
{
  auto const fileName = generator::SrtmTile::GetBase(ms::LatLon(lat, lon));
  return base::JoinPath(dir, fileName + kIsolinesExt);
}

std::string GetIsolinesFilePath(std::string const & countryId, std::string const & dir)
{
  return base::JoinPath(dir, countryId + kIsolinesExt);
}
}  // namespace topography_generator
