#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "base/stl_helpers.hpp"

UNIT_TEST(EditorConfig_TypeDescription)
{
  using EType = feature::Metadata::EType;
  using Fields = editor::TypeAggregatedDescription::FeatureFields;

  Fields const poiInternet = {
    EType::FMD_OPEN_HOURS,
    EType::FMD_PHONE_NUMBER,
    EType::FMD_WEBSITE,
    EType::FMD_INTERNET,
    EType::FMD_EMAIL,
    EType::FMD_LEVEL,
    EType::FMD_CONTACT_FACEBOOK,
    EType::FMD_CONTACT_INSTAGRAM,
    EType::FMD_CONTACT_TWITTER,
    EType::FMD_CONTACT_VK,
    EType::FMD_CONTACT_LINE,
    EType::FMD_CONTACT_FEDIVERSE,
    EType::FMD_CONTACT_BLUESKY,
  };

  pugi::xml_document doc;
  editor::ConfigLoader::LoadFromLocal(doc);

  editor::EditorConfig config;
  config.SetConfig(doc);

  {
    editor::TypeAggregatedDescription desc;
    TEST(!config.GetTypeDescription({"death-star"}, desc), ());
  }
  {
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"amenity-hunting_stand"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST(!desc.IsAddressEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), Fields{EType::FMD_HEIGHT}, ());
  }
  {
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"shop-toys"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), poiInternet, ());
  }
  {
    // Test that amenity-bank is selected as it goes first in config.
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"amenity-bicycle_rental", "amenity-bank"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), poiInternet, ());
  }
  {
    // Testing type inheritance
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"amenity-place_of_worship-christian"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), poiInternet, ());
  }
  {
    // Testing long type inheritance on a fake object
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"tourism-artwork-impresionism-monet"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), Fields{}, ());
  }
  // TODO(mgsergio): Test case with priority="high" when there is one on editor.config.
}

UNIT_TEST(EditorConfig_GetTypesThatCanBeAdded)
{
  pugi::xml_document doc;
  editor::ConfigLoader::LoadFromLocal(doc);

  editor::EditorConfig config;
  config.SetConfig(doc);

  auto const types = config.GetTypesThatCanBeAdded();
  // A sample addable type.
  TEST(find(begin(types), end(types), "amenity-cafe") != end(types), ());
  // A sample line type.
  TEST(find(begin(types), end(types), "highway-primary") == end(types), ());
  // A sample type marked as can_add="no".
  TEST(find(begin(types), end(types), "landuse-cemetery") == end(types), ());
  // A sample type marked as editable="no".
  TEST(find(begin(types), end(types), "aeroway-airport") == end(types), ());
}
