#pragma once

#include "map/catalog_headers_provider.hpp"

#include "partners_api/promo_api.hpp"

#include "indexer/feature_to_osm.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <string>

class DataSource;

namespace search
{
class CityFinder;
}

class PromoDelegate : public promo::Api::Delegate
{
public:
  PromoDelegate(DataSource const & dataSource, search::CityFinder & cityFinder,
                std::shared_ptr<CatalogHeadersProvider> const & headersProvider);

  std::string GetCityId(m2::PointD const & point) override;
  platform::HttpClient::Headers GetHeaders() override;

private:
  DataSource const & m_dataSource;
  search::CityFinder & m_cityFinder;
  std::shared_ptr<CatalogHeadersProvider> m_headersProvider;
  // todo(@a, @m) Drop the unique_ptr and add an IsLoaded() method?
  std::unique_ptr<indexer::FeatureIdToGeoObjectIdOneWay> m_cities;
};
