#include "map/catalog_headers_provider.hpp"

#include "map/bookmark_manager.hpp"

#include <random>
#include <set>

CatalogHeadersProvider::CatalogHeadersProvider(PositionProvider const & positionProvider,
                                               storage::Storage const & storage)
  : m_positionProvider(positionProvider)
  , m_storage(storage)
{
}

void CatalogHeadersProvider::SetBookmarkManager(BookmarkManager const * bookmarkManager)
{
  ASSERT(bookmarkManager != nullptr, ());

  m_bookmarkManager = bookmarkManager;
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
  for (auto const & id : localMaps)
  {
    auto const countryIds = m_storage.GetTopCountryGeoIds(id);
    countries.insert(countryIds.cbegin(), countryIds.cend());

    auto const cityIt = countryToCity.find(id);
    if (cityIt != countryToCity.cend())
      cities.push_back(cityIt->second);
  }
  params.m_countryGeoIds.assign(countries.cbegin(), countries.cend());

  auto ids = m_bookmarkManager->GetAllPaidCategoriesIds();
  if (m_bookmarkManager != nullptr && !ids.empty())
  {
    size_t constexpr kMaxCountOfGuides = 30;
    if (ids.size() > kMaxCountOfGuides)
    {
      static std::mt19937 generator(std::random_device{}());
      std::shuffle(ids.begin(), ids.end(), generator);
      ids.resize(kMaxCountOfGuides);
    }
    params.m_downloadedGuidesIds = std::move(ids);
  }

  return web_api::GetCatalogHeaders(params);
}

std::optional<platform::HttpClient::Header> CatalogHeadersProvider::GetPositionHeader()
{
  if (!m_positionProvider.GetCurrentPosition())
    return {};

  return web_api::GetPositionHeader(*m_positionProvider.GetCurrentPosition());
}
