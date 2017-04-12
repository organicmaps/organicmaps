#include "map/mwm_tree.hpp"

std::unique_ptr<m4::Tree<routing::NumMwmId>> MakeNumMwmTree(routing::NumMwmIds const & numMwmIds,
                                                            storage::CountryInfoGetter const & countryInfoGetter)
{
  auto tree = my::make_unique<m4::Tree<routing::NumMwmId>>();

  numMwmIds.ForEachId([&](routing::NumMwmId numMwmId) {
    auto const & countryName = numMwmIds.GetFile(numMwmId).GetName();
    tree->Add(numMwmId, countryInfoGetter.GetLimitRectForLeaf(countryName));
  });

  return tree;
}
