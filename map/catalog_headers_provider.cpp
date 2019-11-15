#include "map/catalog_headers_provider.hpp"

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
  auto & countries = params.m_countryGeoIds;
  auto & cities = params.m_cityGeoIds;
  for (auto const id : localMaps)
  {
    auto const countryIds = m_storage.GetTopCountryGeoIds(id);
    countries.insert(countries.end(), countryIds.cbegin(), countryIds.cend());

    auto const cityIt = countryToCity.find(id);
    if (cityIt != countryToCity.cend())
      cities.push_back(cityIt->second);
  }

  return web_api::GetCatalogHeaders(params);
}
