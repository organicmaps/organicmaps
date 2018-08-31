#include "storage/routing_helpers.hpp"

#include <memory>

std::unique_ptr<m4::Tree<routing::NumMwmId>> MakeNumMwmTree(
    routing::NumMwmIds const & numMwmIds, storage::CountryInfoGetter const & countryInfoGetter)
{
  auto tree = std::make_unique<m4::Tree<routing::NumMwmId>>();

  numMwmIds.ForEachId([&](routing::NumMwmId numMwmId) {
    auto const & countryName = numMwmIds.GetFile(numMwmId).GetName();
    tree->Add(numMwmId, countryInfoGetter.GetLimitRectForLeaf(countryName));
  });

  return tree;
}

std::shared_ptr<routing::NumMwmIds> CreateNumMwmIds(storage::Storage const & storage)
{
  auto numMwmIds = std::make_shared<routing::NumMwmIds>();
  storage.ForEachCountryFile(
      [&](platform::CountryFile const & file) { numMwmIds->RegisterFile(file); });
  return numMwmIds;
}
