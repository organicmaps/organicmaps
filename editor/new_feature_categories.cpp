#include "new_feature_categories.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>

#include "3party/Alohalytics/src/alohalytics.h"

namespace osm
{
NewFeatureCategories::NewFeatureCategories(editor::EditorConfig const & config)
{
  // TODO(mgsergio): Load types user can create from XML file.
  // TODO: Not every editable type can be created by user.
  // TODO(mgsergio): Store in Settings:: recent history of created types and use them here.
  // Max history items count should be set in the config.
  Classificator const & cl = classif();
  for (auto const & classificatorType : config.GetTypesThatCanBeAdded())
  {
    uint32_t const type = cl.GetTypeByReadableObjectName(classificatorType);
    if (type == 0)
    {
      LOG(LWARNING, ("Unknown type in Editor's config:", classificatorType));
      continue;
    }
    m_types.emplace_back(cl.GetReadableObjectName(type));
  }
}

NewFeatureCategories::NewFeatureCategories(NewFeatureCategories && other)
  : m_index(std::move(other.m_index)), m_types(std::move(other.m_types))
{
}

void NewFeatureCategories::AddLanguage(std::string lang)
{
  auto langCode = CategoriesHolder::MapLocaleToInteger(lang);
  if (langCode == CategoriesHolder::kUnsupportedLocaleCode)
  {
    lang = "en";
    langCode = CategoriesHolder::kEnglishCode;
  }
  if (m_addedLangs.Contains(langCode))
    return;

  auto const & c = classif();
  for (auto const & type : m_types)
  {
    m_index.AddCategoryByTypeAndLang(c.GetTypeByReadableObjectName(type), langCode);
  }

  m_addedLangs.Insert(langCode);
}

NewFeatureCategories::TypeNames NewFeatureCategories::Search(std::string const & query) const
{
  std::vector<uint32_t> resultTypes;
  m_index.GetAssociatedTypes(query, resultTypes);

  auto const & c = classif();
  NewFeatureCategories::TypeNames result(resultTypes.size());
  for (size_t i = 0; i < result.size(); ++i)
  {
    result[i] = c.GetReadableObjectName(resultTypes[i]);
  }

  alohalytics::LogEvent("searchNewFeatureCategory", {{"query", query}});

  return result;
}

NewFeatureCategories::TypeNames const & NewFeatureCategories::GetAllCreatableTypeNames() const
{
  return m_types;
}
}  // namespace osm
