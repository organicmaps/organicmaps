#include "testing/testing.hpp"

#include "drules_test_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "coding/file_writer.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <algorithm>
#include <atomic>
#include <latch>
#include <thread>

namespace drawing_rules_reload_tests
{
using platform::tests_support::ScopedDir;
using platform::tests_support::ScopedFile;

UNIT_TEST(Designer_RuleReloadPreservesTypeIdentitiesAndConcurrentReaders)
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
  TEST_EQUAL(object->GetTypePriority(), priority, ());
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

  std::atomic<bool> done = false;
  std::atomic<bool> consistent = true;
  std::atomic<unsigned> reads = 0;
  std::latch started(1);
  std::thread worker([&]
  {
    while (!done.load())
    {
      if (cl.GetIndexForType(type) != index || cl.GetTypeForIndex(index) != type ||
          cl.GetReadableObjectName(type) != name || cl.GetObject(type) != object ||
          object->GetTypePriority() != priority)
        consistent = false;
      auto types = input;
      types.SortBySpec();
      if (!std::equal(types.begin(), types.end(), expected.begin()))
        consistent = false;
      if (reads.fetch_add(1) == 0)
        started.count_down();
    }
  });
  SCOPE_GUARD(stopWorker, [&]
  {
    done = true;
    if (worker.joinable())
      worker.join();
  });
  started.wait();
  for (int i = 0; i < 10; ++i)
  {
    bool const useEdited = i % 2 != 0;
    auto const & bytes = useEdited ? modified : original;
    {
      FileWriter file(rulesFile.GetFullPath());
      file.Write(bytes.data(), bytes.size());
    }
    classificator::ReloadDrawingRules();
    TEST_EQUAL(object->GetMaxOverlaysPriority(), useEdited ? editedPriority : priority, ());
  }
  done = true;
  worker.join();
  TEST(consistent.load(), ());
  TEST_GREATER(reads.load(), 0, ());
  TEST_EQUAL(object->GetTypePriority(), priority, ());
  TEST_EQUAL(feature::GetDrawableScaleRange(type), range, ());
  TEST_EQUAL(drule::GetCurrentRules().GetBgColor(10), background, ());

  // A new session adopts the edited ordering once it reloads the complete classificator.
  classificator::Load();
  TEST_EQUAL(cl.GetObject(type)->GetTypePriority(), editedPriority, ());
  auto reopened = input;
  reopened.SortBySpec();
  TEST_EQUAL(reopened.front(), type, ());
}

UNIT_TEST(Designer_RejectsChangedTypeSourcesBeforePublication)
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
  });
  classificator::Load();
  ScopedDir dir("designer_changed_type_sources");
  std::string classificatorSource, typesSource;
  GetPlatform().GetReader("classificator.txt")->ReadAsString(classificatorSource);
  GetPlatform().GetReader("types.txt")->ReadAsString(typesSource);
  ScopedFile classificatorFile(base::JoinPath(dir.GetRelativePath(), "classificator.txt"), classificatorSource);
  ScopedFile typesFile(base::JoinPath(dir.GetRelativePath(), "types.txt"), typesSource);
  classificator::CheckTypesCompatible(dir.GetFullPath());
  {
    FileWriter file(typesFile.GetFullPath(), FileWriter::OP_APPEND);
    std::string const changed = "changed mapping";
    file.Write(changed.data(), changed.size());
  }
  bool rejected = false;
  try
  {
    classificator::CheckTypesCompatible(dir.GetFullPath());
  }
  catch (std::runtime_error const &)
  {
    rejected = true;
  }
  TEST(rejected, ());
}
}  // namespace drawing_rules_reload_tests
