#pragma once

#include "editor/editor_config.hpp"

#include "indexer/categories_index.hpp"

#include "base/macros.hpp"

#include "std/cstdint.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace osm
{
// This class holds an index of categories that can be set for a newly added feature.
class NewFeatureCategories
{
public:
  using TName = pair<string, uint32_t>;
  using TNames = vector<TName>;

  NewFeatureCategories(editor::EditorConfig const & config);

  NewFeatureCategories(NewFeatureCategories && other);

  NewFeatureCategories() = default;

  NewFeatureCategories & operator=(NewFeatureCategories && other) = default;

  // Adds all known synonyms in language |lang| for all categories that
  // can be applied to a newly added feature.
  // If one language is added more than once, all the calls except for the
  // first one are ignored.
  // If |lang| is not supported, "en" is used.
  void AddLanguage(string lang);

  // Returns names (in language |queryLang|) and types of categories that have a synonym containing
  // the substring |query| (in any language that was added before).
  // If |lang| is not supported, "en" is used.
  // The returned list is sorted.
  TNames Search(string const & query, string lang) const;

  // Returns all registered names of categories in language |lang| and
  // types corresponding to these names. The language must have been added before.
  // If |lang| is not supported, "en" is used.
  // The returned list is sorted.
  TNames const & GetAllCategoryNames(string const & lang) const;

private:
  indexer::CategoriesIndex m_index;
  vector<uint32_t> m_types;
  map<string, TNames> m_categoriesByLang;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
