#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"
#include "editor/new_feature_categories.hpp"

#include "indexer/classificator_loader.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <string>

// A regional locale translates only a handful of categories: data/categories.txt has 34 "es-MX"
// lines against 518 "es" ones. The index matches the language code exactly, so adding "es-MX" alone
// would leave a Mexican user searching English for everything else.
UNIT_TEST(NewFeatureCategories_RegionalLocaleKeepsBaseLanguage)
{
  classificator::Load();

  pugi::xml_document doc;
  editor::ConfigLoader::LoadFromLocal(doc);
  editor::EditorConfig config;
  config.SetConfig(doc);

  auto search = [&config](std::string const & lang, std::string const & query)
  {
    osm::NewFeatureCategories categories(config);
    categories.AddLanguage(lang);
    return categories.Search(query).size();
  };

  // "Cajero" is only spelled in the base "es" translation of amenity-atm, never in "es-MX".
  TEST_GREATER(search("es", "Cajero"), 0, ("Test premise: the base language must find it"));
  TEST_EQUAL(search("es-MX", "Cajero"), search("es", "Cajero"), ("es-MX must keep the es names"));

  TEST_GREATER(search("pt", "Terminal bancário"), 0, ("Test premise: the base language must find it"));
  TEST_EQUAL(search("pt-BR", "Terminal bancário"), search("pt", "Terminal bancário"), ("pt-BR must keep the pt names"));

  // A script is not a region: "zh-Hant" is a full translation (483 lines against 488 for "zh-Hans"),
  // so it must keep only its own names. Its base "zh" resolves to Simplified Chinese.
  TEST_GREATER(search("zh-Hans", "自动取款机"), 0, ("Test premise: Simplified must find it"));
  TEST_EQUAL(search("zh-Hant", "自动取款机"), 0, ("zh-Hant must not merge the Simplified names"));
  TEST_EQUAL(search("zh-TW", "自动取款机"), 0, ("zh-TW must not merge the Simplified names"));
  TEST_GREATER(search("zh-TW", "自動櫃員機"), 0, ("zh-TW must keep the Traditional names"));
}

UNIT_TEST(NewFeatureCategories_UniqueNames)
{
  classificator::Load();

  editor::EditorConfig config;
  osm::NewFeatureCategories categories(config);

  for (auto const & locale : CategoriesHolder::kLocaleMapping)
  {
    std::string const lang(locale.m_name);
    categories.AddLanguage(lang);
    // GetAllCreatableTypeNames() returns a sorted vector.
    auto names = categories.GetAllCreatableTypeNames();
    auto result = std::unique(names.begin(), names.end());

    if (result != names.end())
    {
      LOG(LWARNING, ("Types duplication detected! The following types are duplicated:"));
      do
      {
        LOG(LWARNING, (*result));
      }
      while (++result != names.end());

      TEST(false, ("Please look at output above"));
    }
  }
}
