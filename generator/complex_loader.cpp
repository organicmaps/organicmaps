#include "generator/complex_loader.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/csv_reader.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <memory>
#include <mutex>
#include <utility>

namespace generator
{
bool IsComplex(tree_node::types::Ptr<HierarchyEntry> const & tree)
{
  size_t constexpr kNumRequiredTypes = 3;

  return tree_node::CountIf(tree, [&](auto const & e)
  {
    auto const & isAttraction = ftypes::AttractionsChecker::Instance();
    return isAttraction(e.m_type);
  }) >= kNumRequiredTypes;
}

storage::CountryId GetCountry(tree_node::types::Ptr<HierarchyEntry> const & tree)
{
  return tree->GetData().m_country;
}

ComplexLoader::ComplexLoader(std::string const & filename)
{
  auto trees = hierarchy::LoadHierachy(filename);
  base::EraseIf(trees, [](auto const & e) { return !IsComplex(e); });
  for (auto const & tree : trees)
    m_forests[GetCountry(tree)].Append(tree);
}

tree_node::Forest<HierarchyEntry> const & ComplexLoader::GetForest(storage::CountryId const & country) const
{
  static tree_node::Forest<HierarchyEntry> const kEmpty;
  auto const it = m_forests.find(country);
  return it == std::cend(m_forests) ? kEmpty : it->second;
}

std::unordered_set<CompositeId> ComplexLoader::GetIdsSet() const
{
  std::unordered_set<CompositeId> set;
  ForEach([&](auto const &, auto const & forest)
  {
    forest.ForEachTree([&](auto const & tree)
    { tree_node::ForEach(tree, [&](auto const & entry) { set.emplace(entry.m_id); }); });
  });
  return set;
}

ComplexLoader const & GetOrCreateComplexLoader(std::string const & filename)
{
  static std::mutex m;
  static std::unordered_map<std::string, ComplexLoader> complexLoaders;

  std::lock_guard<std::mutex> lock(m);
  auto const it = complexLoaders.find(filename);
  if (it != std::cend(complexLoaders))
    return it->second;

  auto const eIt = complexLoaders.emplace(filename, ComplexLoader(filename));
  return eIt.first->second;
}
}  // namespace generator
