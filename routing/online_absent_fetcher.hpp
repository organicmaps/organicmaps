#pragma once

#include "route.hpp"
#include "router.hpp"
#include "routing_mapping.h"

#include "geometry/point2d.hpp"

#include "base/thread.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
using TCountryLocalFileFn = function<shared_ptr<platform::LocalCountryFile>(string const &)>;

/*!
 * \brief The OnlineAbsentCountriesFetcher class incapsulates async fetching the map
 * names from online OSRM server routines.
 */
class OnlineAbsentCountriesFetcher
{
public:
  OnlineAbsentCountriesFetcher(TCountryFileFn const & countryFileFn, TCountryLocalFileFn const & countryLocalFileFn) : m_countryFileFn(countryFileFn), m_countryLocalFileFn(countryLocalFileFn) {}
  void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint);
  void GetAbsentCountries(vector<string> & countries);

private:
  TCountryFileFn const m_countryFileFn;
  TCountryLocalFileFn const m_countryLocalFileFn;
  unique_ptr<threads::Thread> m_fetcherThread;
};
}  // namespace routing
