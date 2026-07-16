#include "new_feature_categories.hpp"

#include "base/string_utils.hpp"
#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include <algorithm>

namespace
{
std::string_view constexpr kRecentlyUsedCategoriesSettingsKey = "RecentCategory";
std::uint16_t constexpr kMaxRecentlyUsedCategoriesCount = 5;
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
  std::sort(m_types.begin(), m_types.end());
}

NewFeatureCategories::NewFeatureCategories(NewFeatureCategories && other) noexcept
  : m_index(std::move(other.m_index))
  , m_types(std::move(other.m_types))
{
  // Do not move m_addedLangs, see Framework::GetEditorCategories() usage.
}

void NewFeatureCategories::AddLanguage(std::string lang)
{
  // A regional locale translates only a handful of categories ("es-MX" has 34 synonyms against 518
  // for "es"), and the index matches the language code exactly, so add the base language too or
  // everything it does not cover would fall back to English names. GetTwine() drops the region but
  // keeps the script: unlike a region, a script is a translation of its own, and merging the other
  // one is not a coverage gap to fill ("zh-Hant" has 483 synonyms, "zh-Hans" 488).
  auto base = languages::GetTwine(lang);
  if (base != lang)
    AddLanguage(std::move(base));

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
  if (category.empty())
    return;
  TypeNames recentlyUsedCategories = GetRecentCategories();
  recentlyUsedCategories.erase(std::remove(recentlyUsedCategories.begin(), recentlyUsedCategories.end(), category),
                               recentlyUsedCategories.end());
  recentlyUsedCategories.insert(recentlyUsedCategories.begin(), category);
  if (recentlyUsedCategories.size() > kMaxRecentlyUsedCategoriesCount)
    recentlyUsedCategories.resize(kMaxRecentlyUsedCategoriesCount);
  std::string result = strings::JoinStrings(recentlyUsedCategories, ";");
  settings::Set(kRecentlyUsedCategoriesSettingsKey, result);
}

NewFeatureCategories::TypeNames NewFeatureCategories::GetRecentCategories() const
{
  std::string current;
  TypeNames recentlyUsedCategories;
  if (!settings::Get(kRecentlyUsedCategoriesSettingsKey, current) || current.empty())
    return {};
  strings::Tokenize(current, ";", [this, &recentlyUsedCategories](std::string_view s)
  {
    if (!s.empty() && std::binary_search(m_types.begin(), m_types.end(), s, std::less<>{}))
      recentlyUsedCategories.emplace_back(s);
  });
  if (recentlyUsedCategories.size() > kMaxRecentlyUsedCategoriesCount)
    recentlyUsedCategories.resize(kMaxRecentlyUsedCategoriesCount);
  return recentlyUsedCategories;
}  // namespace search

}  // namespace osm
