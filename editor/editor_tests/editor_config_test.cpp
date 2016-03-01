#include "testing/testing.hpp"

#include "editor/editor_config.hpp"

#include "std/set.hpp"

using namespace editor;

UNIT_TEST(EditorConfig_TypeDescription)
{
  using EType = feature::Metadata::EType;

  set<EType> const poi = {
    feature::Metadata::FMD_OPEN_HOURS,
    feature::Metadata::FMD_PHONE_NUMBER,
    feature::Metadata::FMD_WEBSITE,
    feature::Metadata::FMD_EMAIL
  };

  EditorConfig config("editor.xml");

  {
    auto const desc = config.GetTypeDescription({"amenity-hunting_stand"});
    TEST(desc.IsNameEditable(), ());
    TEST(!desc.IsAddressEditable(), ());
    TEST_EQUAL(desc.GetEditableFields(), (set<EType>{EType::FMD_HEIGHT}), ());
  }
  {
    auto const desc = config.GetTypeDescription({"shop-toys"});
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    auto fields = poi;
    fields.insert(EType::FMD_INTERNET);
    TEST_EQUAL(desc.GetEditableFields(), fields, ());
  }
  {
    // Select ameniry-bank cause it goes fierst in config
    auto const desc = config.GetTypeDescription({"amenity-bar", "amenity-bank"});
    TEST(desc.IsNameEditable(), ());
    TEST(desc.IsAddressEditable(), ());
    auto fields = poi;
    fields.insert(EType::FMD_OPERATOR);
    TEST_EQUAL(desc.GetEditableFields(), fields, ());
  }
  // TODO(mgsergio): Test case with priority="high" when there is one on editor.xml.
}

UNIT_TEST(EditorConfig_GetTypesThatGenBeAdded)
{
  EditorConfig config("editor.xml");

  auto const types = config.GetTypesThatCanBeAdded();
  TEST(find(begin(types), end(types), "amenity-cafe") != end(types), ());
  TEST(find(begin(types), end(types), "natural-peak") == end(types), ());
}
