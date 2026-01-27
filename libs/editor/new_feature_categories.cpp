#include "new_feature_categories.hpp"

#include "base/string_utils.hpp"
#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "platform/settings.hpp"

#include <algorithm>

namespace
{
std::string_view constexpr kSettingsKey = "RecentCategory";
using Length = uint16_t;
Length constexpr kMaxRecentCategoriesCount = 5;
}  // namespace

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

void NewFeatureCategories::AddToRecentCategories(std::string const & category)
{
  std::string current;
  m_recentCategories.clear();
  m_recentCategories.emplace_back(category);
  if (settings::Get(kSettingsKey, current) && !current.empty())
  {
    strings::Tokenize(current, ";", [this, &category](std::string_view const & s)
    {
      if (!s.empty() && s != category && m_recentCategories.size() < kMaxRecentCategoriesCount)
        m_recentCategories.emplace_back(s);
    });
  }
  std::string result = strings::JoinStrings(m_recentCategories, ";");
  settings::Set(kSettingsKey, result);
}

NewFeatureCategories::TypeNames NewFeatureCategories::GetRecentCategories()
{
  std::string current;
  m_recentCategories.clear();
  if (settings::Get(kSettingsKey, current) && !current.empty())
  {
    strings::Tokenize(current, ";", [this](std::string_view const & s)
    {
      if (!s.empty())
        m_recentCategories.emplace_back(s);
    });
  }
  return m_recentCategories;
}  // namespace search

}  // namespace osm
