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
  NewFeatureCategories(editor::EditorConfig const & config);

  NewFeatureCategories(NewFeatureCategories && other)
    : m_index(move(other.m_index))
    , m_types(move(other.m_types))
    , m_categoryNames(move(other.m_categoryNames))
  {
  }

  NewFeatureCategories() = default;

  NewFeatureCategories & operator=(NewFeatureCategories && other) = default;

  // Adds all known synonyms in language |lang| for all categories that
  // can be applied to a newly added feature.
  void AddLanguage(string const & lang);

  // Returns names (in language |lang|) of categories that have a synonym containing
  // the substring |query| (in any language that was added before).
  // The returned list is sorted.
  vector<string> Search(string const & query, string const & lang) const;

  // Returns all registered names of categories in language |lang|.
  // The returned list is sorted.
  vector<string> GetAllCategoryNames(string const & lang);

private:
  indexer::CategoriesIndex m_index;
  vector<uint32_t> m_types;
  map<string, vector<string>> m_categoryNames;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
