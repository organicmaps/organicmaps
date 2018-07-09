#include "testing/testing.hpp"

#include "editor/config_loader.hpp"
#include "editor/editor_config.hpp"

#include "base/stl_helpers.hpp"

#include "std/set.hpp"

using namespace editor;

UNIT_TEST(EditorConfig_TypeDescription)
{
  using EType = feature::Metadata::EType;
  using TFields = editor::TypeAggregatedDescription::TFeatureFields;

  TFields const poi = {
    feature::Metadata::FMD_OPEN_HOURS,
    feature::Metadata::FMD_PHONE_NUMBER,
    feature::Metadata::FMD_WEBSITE,
    feature::Metadata::FMD_EMAIL,
    feature::Metadata::FMD_LEVEL
  };

  pugi::xml_document doc;
  ConfigLoader::LoadFromLocal(doc);

  EditorConfig config;
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
    TEST_EQUAL(desc.GetEditableFields(), TFields {EType::FMD_HEIGHT}, ());
  }
  {
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"shop-toys"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    auto fields = poi;
    fields.push_back(EType::FMD_INTERNET);
    my::SortUnique(fields);
    TEST_EQUAL(desc.GetEditableFields(), fields, ());
  }
  {
    // Select amenity-bank because it goes first in config.
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"amenity-bar", "amenity-bank"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    auto fields = poi;
    fields.push_back(EType::FMD_OPERATOR);
    my::SortUnique(fields);
    TEST_EQUAL(desc.GetEditableFields(), fields, ());
  }
  {
    // Testing type inheritance
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"amenity-place_of_worship-christian"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), poi, ());
  }
  {
    // Testing long type inheritance on a fake object
    editor::TypeAggregatedDescription desc;
    TEST(config.GetTypeDescription({"tourism-artwork-impresionism-monet"}, desc), ());
    TEST(desc.IsNameEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), TFields {}, ());
  }
  // TODO(mgsergio): Test case with priority="high" when there is one on editor.config.
}

UNIT_TEST(EditorConfig_GetTypesThatCanBeAdded)
{
  pugi::xml_document doc;
  ConfigLoader::LoadFromLocal(doc);

  EditorConfig config;
  config.SetConfig(doc);

  auto const types = config.GetTypesThatCanBeAdded();
  TEST(find(begin(types), end(types), "amenity-cafe") != end(types), ());
  TEST(find(begin(types), end(types), "natural-peak") == end(types), ());
  // Marked as "editable=no".
  TEST(find(begin(types), end(types), "aeroway-airport") == end(types), ());
}
