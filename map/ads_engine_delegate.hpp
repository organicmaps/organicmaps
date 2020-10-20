#pragma once

#include "map/purchase.hpp"

#include "partners_api/ads/ads_engine.hpp"
#include "partners_api/promo_api.hpp"
#include "partners_api/taxi_engine.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"
#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"

#include <string>

namespace ads
{
class AdsEngineDelegate : public Engine::Delegate
{
public:
  AdsEngineDelegate(storage::CountryInfoGetter const & infoGetter, storage::Storage const & storage,
                    promo::Api const & promoApi, Purchase const & purchase,
                    taxi::Engine const & taxiEngine);

  // Engine::Delegate overrides:
  bool IsAdsRemoved() const override;

  // DownloadOnMapContainer::Delegate overrides:
  storage::CountryId GetCountryId(m2::PointD const & pos) override;
  storage::CountryId GetTopmostParentFor(storage::CountryId const & countryId) const override;
  std::string GetLinkForCountryId(storage::CountryId const & countryId) const override;
  std::vector<taxi::Provider::Type> GetTaxiProvidersAtPos(m2::PointD const & pos) const override;

private:
  storage::CountryInfoGetter const & m_countryInfoGetter;
  storage::Storage const & m_storage;
  promo::Api const & m_promoApi;
  Purchase const & m_purchase;
  taxi::Engine const & m_taxiEngine;
};
}  // namespace ads
