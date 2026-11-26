#include "testing/testing.hpp"

#include "drules_test_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"

#include <algorithm>

namespace drawing_rules_reload_tests
{
using platform::tests_support::ScopedFile;

UNIT_TEST(Designer_RuleReloadPreservesTypeIdentitiesAndUpdatesPriorities)
{
  auto & reader = GetStyleReader();
  auto const style = reader.GetCurrentStyle();
  auto const designer = reader.IsDesignerMode();
  reader.SetDesignerMode(true);
  reader.SetCurrentStyle(MapStyleDefaultLight);
  SCOPE_GUARD(restore, [&]
  {
    reader.SetDesignerMode(designer);
    reader.SetCurrentStyle(style);
    classificator::Load();
  });
  classificator::Load();

  auto & cl = classif();
  auto const type = cl.GetTypeByPath({"amenity", "bench"});
  auto const index = cl.GetIndexForType(type);
  auto const object = cl.GetObject(type);
  auto const name = cl.GetReadableObjectName(type);
  auto const range = feature::GetDrawableScaleRange(type);
  auto const priority = object->GetMaxOverlaysPriority();
  auto const background = drule::GetCurrentRules().GetBgColor(10);
  feature::TypesHolder input(feature::GeomType::Point);
  input.Add(type);
  input.Add(cl.GetTypeByPath({"amenity", "hospital"}));
  input.Add(cl.GetTypeByPath({"building"}));
  auto expected = input;
  expected.SortBySpec();

  auto format = drule::DecodeRules(MapStyleDefaultLight);
  std::string const original = drules_format_tests::EncodeForTests(format);
  int const editedPriority = std::max(priority, cl.GetObject(expected.front())->GetMaxOverlaysPriority()) + 1000;
  bool edited = false;
  for (auto & entry : format.types)
    if (entry.name == "amenity-bench")
      for (auto & element : entry.elements)
        if (element.symbol)
        {
          element.symbol->priority = editedPriority;
          edited = true;
        }
  TEST(edited, ());
  std::string const modified = drules_format_tests::EncodeForTests(format);
  WritableDirChanger dir("designer_rule_reload");
  ScopedFile rulesFile("drules_default.bin", original);

  for (int i = 0; i < 10; ++i)
  {
    bool const useEdited = i % 2 != 0;
    auto const & bytes = useEdited ? modified : original;
    {
      FileWriter file(rulesFile.GetFullPath());
      file.Write(bytes.data(), bytes.size());
    }
    classificator::ReloadDrawingRules();
    TEST_EQUAL(cl.GetIndexForType(type), index, ());
    TEST_EQUAL(cl.GetTypeForIndex(index), type, ());
    TEST_EQUAL(cl.GetReadableObjectName(type), name, ());
    TEST_EQUAL(cl.GetObject(type), object, ());
    TEST_EQUAL(object->GetMaxOverlaysPriority(), useEdited ? editedPriority : priority, ());
    auto types = input;
    types.SortBySpec();
    if (useEdited)
      TEST_EQUAL(types.front(), type, ());
    else
      TEST(std::equal(types.begin(), types.end(), expected.begin()), ());
  }
  TEST_EQUAL(feature::GetDrawableScaleRange(type), range, ());
  TEST_EQUAL(drule::GetCurrentRules().GetBgColor(10), background, ());
}
}  // namespace drawing_rules_reload_tests
