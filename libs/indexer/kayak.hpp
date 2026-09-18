#pragma once

#include <cstdint>
#include <ctime>
#include <string>

namespace osm
{

std::string GetKayakHotelURL(std::string const & countryIsoCode, uint64_t kayakHotelId,
                             std::string const & kayakHotelName, uint64_t kayakCityId, time_t firstDay, time_t lastDay);

std::string GetKayakHotelURLFromURI(std::string const & countryIsoCode, std::string const & uri, time_t firstDay,
                                    time_t lastDay);
}  // namespace osm
