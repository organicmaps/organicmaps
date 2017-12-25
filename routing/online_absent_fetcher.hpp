#pragma once

#include "routing/checkpoints.hpp"
#include "routing/router.hpp"

#include "geometry/point2d.hpp"

#include "base/thread.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace routing
{
using TCountryLocalFileFn = function<bool(string const &)>;

class IOnlineFetcher
{
public:
  virtual ~IOnlineFetcher() = default;
  virtual void GenerateRequest(Checkpoints const &) = 0;
  virtual void GetAbsentCountries(vector<string> & countries) = 0;
};

/*!
 * \brief The OnlineAbsentCountriesFetcher class incapsulates async fetching the map
 * names from online OSRM server routines.
 */
class OnlineAbsentCountriesFetcher : public IOnlineFetcher
{
public:
  OnlineAbsentCountriesFetcher(TCountryFileFn const &, TCountryLocalFileFn const &);

  // IOnlineFetcher overrides:
  void GenerateRequest(Checkpoints const &) override;
  void GetAbsentCountries(vector<string> & countries) override;

private:
  bool AllPointsInSameMwm(Checkpoints const &) const;

  TCountryFileFn const m_countryFileFn;
  TCountryLocalFileFn const m_countryLocalFileFn;
  unique_ptr<threads::Thread> m_fetcherThread;
};
}  // namespace routing
