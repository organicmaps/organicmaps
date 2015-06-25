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
  OnlineAbsentFetcher(TCountryFileFn const & fn) : m_countryFunction(fn) {}
  void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint);
  void GetAbsentCountries(vector<string> & countries);

private:
  TCountryFileFn const m_countryFunction;
  threads::Thread m_fetcherThread;
};
}  // namespace routing
