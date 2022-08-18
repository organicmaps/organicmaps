#pragma once

#include "editor/editor_config.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/categories_index.hpp"

#include "base/macros.hpp"
#include "base/small_set.hpp"

#include <string>
#include <vector>

namespace osm
{
// This class holds an index of categories that can be set for a newly added feature.
class NewFeatureCategories
{
public:
  using TypeName = std::string;
  using TypeNames = std::vector<TypeName>;

  NewFeatureCategories() = default;
  explicit NewFeatureCategories(editor::EditorConfig const & config);

  NewFeatureCategories(NewFeatureCategories && other) noexcept;
  NewFeatureCategories & operator=(NewFeatureCategories && other) = default;

  // Adds all known synonyms in language |lang| for all categories that
  // can be applied to a newly added feature.
  // If one language is added more than once, all the calls except for the
  // first one are ignored.
  // If |lang| is not supported, "en" is used.
  void AddLanguage(std::string lang);

  // Returns names (in language |queryLang|) and types of categories that have a synonym containing
  // the substring |query| (in any language that was added before).
  // If |lang| is not supported, "en" is used.
  // The returned list is sorted.
  TypeNames Search(std::string const & query) const;

  // Returns all registered classifier category types (GetReadableObjectName).
  TypeNames const & GetAllCreatableTypeNames() const { return m_types; }

private:
  using Langs = base::SmallSet<CategoriesHolder::kLocaleMapping.size() + 1>;

  indexer::CategoriesIndex m_index;
  Langs m_addedLangs;
  TypeNames m_types;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
