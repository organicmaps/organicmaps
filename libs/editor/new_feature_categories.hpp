#pragma once

#include "editor/editor_config.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/categories_index.hpp"

#include "base/macros.hpp"
#include "base/small_set.hpp"

#include <functional>
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

  NewFeatureCategories(NewFeatureCategories && other) noexcept = default;
  NewFeatureCategories & operator=(NewFeatureCategories && other) = default;

  // Adds all known synonyms in language |lang| for all categories that
  // can be applied to a newly added feature.
  // If one language is added more than once, all the calls except for the
  // first one are ignored.
  // If |lang| is not supported, "en" is used.
  void AddLanguage(std::string lang);

  // Adds the names shown by the picker to the same index as search keywords. The resolver receives
  // a readable classifier identifier and is called synchronously for every creatable type.
  // Call once for a picker's display locale; use a new instance when that locale changes.
  void AddLocalizedNames(std::function<std::string(TypeName const &)> const & localize);

  // Returns readable classifier identifiers whose indexed names contain every normalized query token
  // as a substring. Matches display names and synonyms in every language added with AddLanguage().
  TypeNames Search(std::string const & query) const;

  TypeNames GetRecentCategories() const;

  void AddToRecentCategories(std::string const & category);

  // Returns all registered classifier category types (GetReadableObjectName).
  // The returned list is sorted.
  TypeNames const & GetAllCreatableTypeNames() const { return m_types; }

private:
  using Langs = base::SmallSet<CategoriesHolder::kLocaleMapping.size() + 1>;

  indexer::CategoriesIndex m_index;
  Langs m_addedLangs;
  TypeNames m_types;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
