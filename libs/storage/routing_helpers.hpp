#pragma once

#include "routing_common/num_mwm_id.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "geometry/tree4d.hpp"

#include <memory>

namespace routing
{
std::unique_ptr<m4::Tree<routing::NumMwmId>> MakeNumMwmTree(NumMwmIds const & numMwmIds,
                                                            storage::CountryInfoGetter const & countryInfoGetter);
std::shared_ptr<NumMwmIds> CreateNumMwmIds(storage::Storage const & storage);
}  // namespace routing
