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
  void SetLinkForCountryId(std::string const & linkForCountryId)
  {
    m_linkForCountryId = linkForCountryId;
  }

  // ads::DownloadOnMapContainer::Delegate
  storage::CountryId GetCountryId(m2::PointD const & pos) override { return m_countryId; }
  storage::CountriesVec GetTopmostNodesFor(storage::CountryId const & mwmId) const override
  {
    return m_topmostNodes;
  }
  std::string GetLinkForCountryId(storage::CountryId const & countryId) const override
  {
    return m_linkForCountryId;
  }

private:
  storage::CountryId m_countryId;
  storage::CountriesVec m_topmostNodes;
  std::string m_linkForCountryId;
};
