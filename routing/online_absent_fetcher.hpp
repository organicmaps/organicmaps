#pragma once

#include "route.hpp"
#include "router.hpp"
#include "routing_mapping.h"

#include "base/thread.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
class OnlineAbsentFetcher
{
public:
  OnlineAbsentFetcher(TCountryFileFn const & countryFileFn, TCountryLocalFileFn const & countryLocalFileFn) : m_countryFileFn(countryFileFn), m_countryLocalFileFn(countryLocalFileFn) {}
  void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint);
  void GetAbsentCountries(vector<string> & countries);

private:
  TCountryFileFn const m_countryFileFn;
  TCountryLocalFileFn const m_countryLocalFileFn;
  threads::Thread m_fetcherThread;
};
}  // namespace routing
