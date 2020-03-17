#pragma once

#include "partners_api/ads/ads_base.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"

#include <string>

class DownloadOnMapContainerDelegateForTesting : public ads::DownloadOnMapContainer::Delegate
{
public:
  void SetCountryId(storage::CountryId const & countryId) { m_countryId = countryId; }
  void SetTopmostNodes(storage::CountriesVec const & topmostNodes)
  {
    m_topmostNodes = topmostNodes;
  }
  void SetTopCityGeoId(std::string const & topCityGeoId) { m_topCityGeoId = topCityGeoId; }
  void SetLinkForGeoId(std::string const & linkForGeoId) { m_linkForGeoId = linkForGeoId; }

  // ads::DownloadOnMapContainer::Delegate
  storage::CountryId GetCountryId(m2::PointD const & pos) override { return m_countryId; }
  storage::CountriesVec GetTopmostNodesFor(storage::CountryId const & mwmId) const override
  {
    return m_topmostNodes;
  }
  std::string GetMwmTopCityGeoId(storage::CountryId const & mwmId) const override
  {
    return m_topCityGeoId;
  }

  std::string GetLinkForGeoId(std::string const & id) const override { return m_linkForGeoId; }

private:
  storage::CountryId m_countryId;
  storage::CountriesVec m_topmostNodes;
  std::string m_topCityGeoId;
  std::string m_linkForGeoId;
};
