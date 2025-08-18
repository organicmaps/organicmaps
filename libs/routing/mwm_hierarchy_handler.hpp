#pragma once

#include "routing/route_weight.hpp"
#include "routing/router.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{

/// Class for calculating penalty while crossing country borders. Also finds parent country for mwm.
class MwmHierarchyHandler
{
public:
  // Used in tests only.
  MwmHierarchyHandler() = default;
  // Used in IndexRouter.
  MwmHierarchyHandler(std::shared_ptr<NumMwmIds> numMwmIds, CountryParentNameGetterFn countryParentNameGetterFn);

  bool HasCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2);
  RouteWeight GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2);

private:
  /// @return Parent country name for \a mwmId.
  std::string GetMwmName(NumMwmId mwmId) const;
  std::string GetParentCountry(NumMwmId mwmId) const;
  std::string const & GetParentCountryCached(NumMwmId mwmId);

  std::shared_ptr<NumMwmIds> m_numMwmIds;
  CountryParentNameGetterFn m_countryParentNameGetterFn;

  using MwmToCountry = std::unordered_map<NumMwmId, std::string>;
  MwmToCountry m_mwmCountriesCache;
};
}  // namespace routing
