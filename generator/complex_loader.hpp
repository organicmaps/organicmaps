#pragma once

#include "generator/hierarchy_entry.hpp"

#include "indexer/complex/tree_node.hpp"

#include "storage/storage_defines.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace generator
{
namespace complex
{
// Feature ids.
using Ids = std::vector<uint32_t>;
}  // namespace complex

// ComplexLoader loads complexes from hierarchy source file and provides easy access.
class ComplexLoader
{
public:
  explicit ComplexLoader(std::string const & filename);

  tree_node::Forest<HierarchyEntry> const & GetForest(storage::CountryId const & country) const;

  // fn accepts country name and tree.
  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    for (auto const & pair : m_forests)
      fn(pair.first, pair.second);
  }

  std::unordered_set<CompositeId> GetIdsSet() const;

private:
  std::unordered_map<storage::CountryId, tree_node::Forest<HierarchyEntry>> m_forests;
};

// Returns true if hierarchy tree is complex; otherwise returns false.
// Complex is defined at https://confluence.mail.ru/display/MAPSME/Complexes.
bool IsComplex(tree_node::types::Ptr<HierarchyEntry> const & tree);

// Returns country id of hierarchy.
storage::CountryId GetCountry(tree_node::types::Ptr<HierarchyEntry> const & tree);

// Returns initilized ComplexLoader by filename.
// It loads only at the time of the first call.
ComplexLoader const & GetOrCreateComplexLoader(std::string const & filename);

template <typename Fn>
tree_node::Forest<complex::Ids> TraformToIdsForest(tree_node::Forest<HierarchyEntry> const & forest, Fn && fn)
{
  tree_node::Forest<complex::Ids> res;
  forest.ForEachTree([&](auto const & tree) { res.Append(tree_node::TransformToTree(tree, std::forward<Fn>(fn))); });
  return res;
}
}  // namespace generator
