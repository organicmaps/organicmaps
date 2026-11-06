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

  // Indexes synonyms in |lang| for creatable types, including the base language for regional locales.
  // Unsupported locales use English. Repeated locale codes do not add duplicate index entries.
  void AddLanguage(std::string lang);

  // Adds the names shown by the picker to the same index as search keywords. The resolver receives
  // a readable classifier identifier and is called synchronously for every creatable type.
  // Call once for a picker's display locale; use a new instance when that locale changes.
  void AddLocalizedNames(std::function<std::string(TypeName const &)> const & localize);

  // Returns readable classifier identifiers whose indexed names contain every normalized query token
  // as a substring. Tokens may match display names or synonyms in different languages.
  // Results are ordered by classifier type.
  TypeNames Search(std::string const & query) const;

  TypeNames GetRecentCategories() const;

  void AddToRecentCategories(std::string const & category);

  // Returns readable classifier identifiers allowed for new features, sorted lexicographically.
  TypeNames const & GetAllCreatableTypeNames() const { return m_types; }

private:
  using Langs = base::SmallSet<CategoriesHolder::kLocaleMapping.size() + 1>;

  indexer::CategoriesIndex m_index;
  Langs m_addedLangs;
  TypeNames m_types;
  bool m_namesAdded = false;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
