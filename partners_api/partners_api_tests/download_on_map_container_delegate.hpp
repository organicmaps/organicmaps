#pragma once

#include "partners_api/ads/ads_base.hpp"

#include "storage/storage_defines.hpp"

#include "geometry/point2d.hpp"

#include <string>

class DownloadOnMapContainerDelegateForTesting : public ads::DownloadOnMapContainer::Delegate
{
public:
  void SetCountryId(storage::CountryId const & countryId) { m_countryId = countryId; }
  void SetTopmostParent(storage::CountryId const & topmostParent)
  {
    m_topmostParent = topmostParent;
  }
  void SetLinkForCountryId(std::string const & linkForCountryId)
  {
    m_linkForCountryId = linkForCountryId;
  }

  // ads::DownloadOnMapContainer::Delegate
  storage::CountryId GetCountryId(m2::PointD const & pos) override { return m_countryId; }
  storage::CountryId GetTopmostParentFor(storage::CountryId const & mwmId) const override
  {
    return m_topmostParent;
  }
  std::string GetLinkForCountryId(storage::CountryId const & countryId) const override
  {
    return m_linkForCountryId;
  }

private:
  storage::CountryId m_countryId;
  storage::CountryId m_topmostParent;
  std::string m_linkForCountryId;
};
