#pragma once

#include "routing/checkpoints.hpp"
#include "routing/router.hpp"

#include "geometry/point2d.hpp"

#include "base/thread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace routing
{
using TCountryLocalFileFn = std::function<bool(std::string const &)>;

class IOnlineFetcher
{
public:
  virtual ~IOnlineFetcher() = default;
  virtual void GenerateRequest(Checkpoints const &) = 0;
  virtual void GetAbsentCountries(std::vector<std::string> & countries) = 0;
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
  void GetAbsentCountries(std::vector<std::string> & countries) override;

private:
  bool AllPointsInSameMwm(Checkpoints const &) const;

  TCountryFileFn const m_countryFileFn;
  TCountryLocalFileFn const m_countryLocalFileFn;
  std::unique_ptr<threads::Thread> m_fetcherThread;
};
}  // namespace routing
