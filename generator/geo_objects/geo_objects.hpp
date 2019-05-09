#pragma once

#include <string>

namespace generator
{
namespace geo_objects
{
// This function generates key-value pairs for geo objects.
// First, we try to generate key-value pairs only for houses, since we cannot say anything about poi.
// In this step, we need key-value pairs for the regions and the index for the regions.
// Then we build an index for houses. And then we finish building key-value pairs for poi using
// this index for houses.
// |allowAddresslessForCountries| specifies countries for which addressless buldings are constructed
// in index and key-value files. Countries are specified by osm's default local name (or part of name)
// separated by commas. Default value is '*' (for all countries).
bool GenerateGeoObjects(std::string const & pathInRegionsIndex,
                        std::string const & pathInRegionsKv,
                        std::string const & pathInGeoObjectsTmpMwm,
                        std::string const & pathOutIdsWithoutAddress,
                        std::string const & pathOutGeoObjectsKv,
                        std::string const & allowAddresslessForCountries,
                        bool verbose, size_t threadsCount);
}  // namespace geo_objects
}  // namespace generator
