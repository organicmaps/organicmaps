#include "storage/routing_helpers.hpp"

namespace routing
{

std::unique_ptr<m4::Tree<routing::NumMwmId>> MakeNumMwmTree(NumMwmIds const & numMwmIds,
                                                            storage::CountryInfoGetter const & countryInfoGetter)
{
  auto tree = std::make_unique<m4::Tree<NumMwmId>>();

  numMwmIds.ForEachId([&](NumMwmId numMwmId)
  {
    auto const & countryName = numMwmIds.GetFile(numMwmId).GetName();
    tree->Add(numMwmId, countryInfoGetter.GetLimitRectForLeaf(countryName));
  });

  return tree;
}

std::shared_ptr<routing::NumMwmIds> CreateNumMwmIds(storage::Storage const & storage)
{
  auto numMwmIds = std::make_shared<routing::NumMwmIds>();
  storage.ForEachCountry([&](storage::Country const & country) { numMwmIds->RegisterFile(country.GetFile()); });
  return numMwmIds;
}

}  // namespace routing
