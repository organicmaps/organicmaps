#pragma once

#include "routing/route_weight.hpp"
#include "routing/router.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace routing
{
using MwmToCountry = std::unordered_map<NumMwmId, std::string>;

// Class for calculating penalty while crossing country borders. Also finds parent country for mwm.
class MwmHierarchyHandler
{
public:
  MwmHierarchyHandler(std::shared_ptr<NumMwmIds> numMwmIds,
                      CountryParentNameGetterFn countryParentNameGetterFn);

  RouteWeight GetCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2);

private:
  bool HasCrossBorderPenalty(NumMwmId mwmId1, NumMwmId mwmId2);

  // Returns parent country name for |mwmId|.
  std::string const & GetParentCountryByMwmId(NumMwmId mwmId);

  std::shared_ptr<NumMwmIds> m_numMwmIds = nullptr;
  CountryParentNameGetterFn m_countryParentNameGetterFn = nullptr;
  MwmToCountry m_mwmCountriesCache;
};
}  // namespace routing
