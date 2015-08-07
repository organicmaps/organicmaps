#pragma once

#include "routing_mapping.hpp"

#include "geometry/point2d.hpp"

#include "base/thread.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"


namespace routing
{
using TCountryLocalFileFn = function<shared_ptr<platform::LocalCountryFile>(string const &)>;

class IOnlineFetcher
{
public:
  virtual ~IOnlineFetcher() = default;
  virtual void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint) = 0;
  virtual void GetAbsentCountries(vector<string> & countries) = 0;
};

/*!
 * \brief The OnlineAbsentCountriesFetcher class incapsulates async fetching the map
 * names from online OSRM server routines.
 */
class OnlineAbsentCountriesFetcher : public IOnlineFetcher
{
public:
  OnlineAbsentCountriesFetcher(TCountryFileFn const & countryFileFn,
                               TCountryLocalFileFn const & countryLocalFileFn)
    : m_countryFileFn(countryFileFn), m_countryLocalFileFn(countryLocalFileFn)
  {
  }

  // IOnlineFetcher overrides:
  void GenerateRequest(m2::PointD const & startPoint, m2::PointD const & finalPoint) override;
  void GetAbsentCountries(vector<string> & countries) override;

private:
  TCountryFileFn const m_countryFileFn;
  TCountryLocalFileFn const m_countryLocalFileFn;
  unique_ptr<threads::Thread> m_fetcherThread;
};
}  // namespace routing
