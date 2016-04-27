#include "new_feature_categories.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"

namespace osm
{
NewFeatureCategories::NewFeatureCategories(editor::EditorConfig const & config)
{
  // TODO(mgsergio): Load types user can create from XML file.
  // TODO: Not every editable type can be created by user.
  // TODO(mgsergio): Store in Settings:: recent history of created types and use them here.
  // Max history items count shoud be set in the config.
  Classificator const & cl = classif();
  for (auto const & classificatorType : config.GetTypesThatCanBeAdded())
  {
    uint32_t const type = cl.GetTypeByReadableObjectName(classificatorType);
    if (type == 0)
    {
      LOG(LWARNING, ("Unknown type in Editor's config:", classificatorType));
      continue;
    }
    m_types.push_back(type);
  }
}

void NewFeatureCategories::AddLanguage(string const & lang)
{
  auto const langCode = CategoriesHolder::MapLocaleToInteger(lang);
  NewFeatureCategories::TNames names;
  names.reserve(m_types.size());
  for (auto const & type : m_types)
  {
    m_index.AddCategoryByTypeAndLang(type, langCode);
    names.emplace_back(m_index.GetCategoriesHolder()->GetReadableFeatureType(type, langCode), type);
  }
  my::SortUnique(names);
  m_categoriesByLang[lang] = names;
}

NewFeatureCategories::TNames NewFeatureCategories::Search(string const & query,
                                                          string const & lang) const
{
  auto const langCode = CategoriesHolder::MapLocaleToInteger(lang);
  vector<uint32_t> resultTypes;
  m_index.GetAssociatedTypes(query, resultTypes);

  NewFeatureCategories::TNames result(resultTypes.size());
  for (size_t i = 0; i < result.size(); ++i)
    result[i] = {m_index.GetCategoriesHolder()->GetReadableFeatureType(resultTypes[i], langCode),
                 resultTypes[i]};
  my::SortUnique(result);
  return result;
}

NewFeatureCategories::TNames const & NewFeatureCategories::GetAllCategoryNames(
    string const & lang) const
{
  auto const it = m_categoriesByLang.find(lang);
  CHECK(it != m_categoriesByLang.end(), ());
  return it->second;
}
}  // namespace osm
