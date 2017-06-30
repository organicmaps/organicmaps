#pragma once

#include "partners_api/taxi_engine.hpp"

namespace storage
{
class Storage;
class CountryInfoGetter;
}

namespace search
{
class CityFinder;
}

class TaxiDelegate : public taxi::Delegate
{
public:
  TaxiDelegate(storage::Storage const & st, storage::CountryInfoGetter const & ig, search::CityFinder & cf);

  storage::TCountriesVec GetCountryIds(ms::LatLon const & latlon) override;
  std::string GetCityName(ms::LatLon const & latlon) override;

private:
  storage::Storage const & m_storage;
  storage::CountryInfoGetter const & m_infoGetter;
  search::CityFinder & m_cityFinder;
};
