#include "map/catalog_headers_provider.hpp"

#include <set>

CatalogHeadersProvider::CatalogHeadersProvider(PositionProvider const & positionProvider,
                                               storage::Storage const & storage)
  : m_positionProvider(positionProvider)
  , m_storage(storage)
{
}

platform::HttpClient::Headers CatalogHeadersProvider::GetHeaders()
{
  web_api::HeadersParams params;
  params.m_currentPosition = m_positionProvider.GetCurrentPosition();

  storage::CountriesVec localMaps;
  m_storage.GetLocalRealMaps(localMaps);
  auto const & countryToCity = m_storage.GetMwmTopCityGeoIds();
  std::set<base::GeoObjectId> countries;
  auto & cities = params.m_cityGeoIds;
  for (auto const id : localMaps)
  {
    auto const countryIds = m_storage.GetTopCountryGeoIds(id);
    countries.insert(countryIds.cbegin(), countryIds.cend());

    auto const cityIt = countryToCity.find(id);
    if (cityIt != countryToCity.cend())
      cities.push_back(cityIt->second);
  }
  params.m_countryGeoIds.assign(countries.cbegin(), countries.cend());

  return web_api::GetCatalogHeaders(params);
}
