#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"
#include "editor/new_feature_categories.hpp"

#include "indexer/classificator_loader.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <string>

// Regional translations cover only some categories. The index matches locale codes exactly,
// so a regional locale must also keep the base language's names.
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

  // Chinese scripts have independent translations, so Traditional locales must not index Simplified names.
  TEST_GREATER(search("zh-Hans", "自动取款机"), 0, ("Test premise: Simplified must find it"));
  TEST_EQUAL(search("zh-Hant", "自动取款机"), 0, ("zh-Hant must not merge the Simplified names"));
  TEST_EQUAL(search("zh-TW", "自动取款机"), 0, ("zh-TW must not merge the Simplified names"));
  TEST_GREATER(search("zh-TW", "自動櫃員機"), 0, ("zh-TW must keep the Traditional names"));
}

UNIT_TEST(NewFeatureCategories_RepeatedTypeRecords)
{
  classificator::Load();

  pugi::xml_document doc;
  TEST(doc.load_string(R"(
    <omaps><editor><types><type id="shop-car_repair" /></types></editor></omaps>
  )"),
       ());
  editor::EditorConfig config;
  config.SetConfig(doc);
  osm::NewFeatureCategories categories(config);
  categories.AddLanguage("en");

  osm::NewFeatureCategories::TypeNames const expected = {"shop-car_repair"};
  TEST_EQUAL(categories.GetAllCreatableTypeNames(), expected, ());
  for (auto const query : {"Car Repair Workshop", "garage", "Tyre Repair", "tyres", "puncture repair",
                           "tyre puncture repair", "flat tyre repair", "tyre replacement"})
    TEST_EQUAL(categories.Search(query), expected, (query));
}

UNIT_TEST(NewFeatureCategories_WelshTypeNames)
{
  classificator::Load();

  pugi::xml_document doc;
  editor::ConfigLoader::LoadFromLocal(doc);
  editor::EditorConfig config;
  config.SetConfig(doc);

  osm::NewFeatureCategories categories(config);
  categories.AddLanguage("cy-GB");
  categories.AddLanguage("en");

  struct TestCase
  {
    char const * m_query;
    char const * m_type;
  };
  TestCase const cases[] = {
      {"Crefft Llaw", "craft-handicraft"},
      {"Gwresogi, Awyru ac Aerdymheru", "craft-hvac"},
      {"Canolfan Roi Gwaed", "healthcare-blood_donation"},
      {"Canolfan Gasglu Samplau", "healthcare-sample_collection"},
      {"Ffynnon Olew neu Nwy", "man_made-petroleum_well"},
      {"Siop Rannau Ceir", "shop-car_parts"},
      {"Siop Gymhorthion Clyw", "shop-hearing_aids"},
      {"Siop Hynafiaethau", "shop-antiques"},
      {"Siop Gyflenwadau Trydanol", "shop-electrical"},
      {"Siop Gyflenwadau Meddygol", "shop-medical_supply"},
      {"Siop Atchwanegiadau Maethol", "shop-nutrition_supplements"},
      {"Siop Gyflenwadau Gwnïo", "shop-sewing"},
      {"Cyflenwadau Masnachol", "shop-trade"},
  };

  // The displayed Welsh names must find their categories when pasted into the editor search.
  for (auto const & test : cases)
  {
    auto const results = categories.Search(test.m_query);
    TEST(std::find(results.begin(), results.end(), test.m_type) != results.end(), (test.m_query, test.m_type, results));
  }
}

UNIT_TEST(NewFeatureCategories_UniqueNames)
{
  classificator::Load();

  pugi::xml_document doc;
  editor::ConfigLoader::LoadFromLocal(doc);
  editor::EditorConfig config;
  config.SetConfig(doc);
  osm::NewFeatureCategories categories(config);

  // Classifier identifiers are independent of the added search languages and are sorted by the constructor.
  auto const & names = categories.GetAllCreatableTypeNames();
  TEST(!names.empty(), ());
  TEST(std::adjacent_find(names.begin(), names.end()) == names.end(), (names));
}
