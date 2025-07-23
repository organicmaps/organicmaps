#include "new_feature_categories.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"

#include <algorithm>

namespace osm
{
NewFeatureCategories::NewFeatureCategories(editor::EditorConfig const & config)
{
  Classificator const & c = classif();
  for (auto const & clType : config.GetTypesThatCanBeAdded())
  {
    uint32_t const type = c.GetTypeByReadableObjectName(clType);
    if (type == 0)
    {
      LOG(LWARNING, ("Unknown type in Editor's config:", clType));
      continue;
    }
    m_types.emplace_back(clType);
  }
}

NewFeatureCategories::NewFeatureCategories(NewFeatureCategories && other) noexcept
  : m_index(std::move(other.m_index))
  , m_types(std::move(other.m_types))
{
  // Do not move m_addedLangs, see Framework::GetEditorCategories() usage.
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
    m_index.AddCategoryByTypeAndLang(c.GetTypeByReadableObjectName(type), langCode);

  m_addedLangs.Insert(langCode);
}

NewFeatureCategories::TypeNames NewFeatureCategories::Search(std::string const & query) const
{
  std::vector<uint32_t> resultTypes;
  m_index.GetAssociatedTypes(query, resultTypes);

  auto const & c = classif();
  NewFeatureCategories::TypeNames result(resultTypes.size());
  for (size_t i = 0; i < result.size(); ++i)
    result[i] = c.GetReadableObjectName(resultTypes[i]);

  return result;
}

}  // namespace osm
